#define CATCH_CONFIG_MAIN

#include <cassert>
#include <ostream>
#include <sstream>
#include <string>

#include "clang/Tooling/Tooling.h"

#include "catch2/catch.hpp"

#include "CreateWrapper.hpp"
#include "FundamentalTypes.hpp"
#include "Options.hpp"
#include "String.hpp"

namespace
{

class Snippet
{
public:
  Snippet(std::string const &Code)
  : Code_(Code),
    WrappedCode_(wrapCode(Code))
  {}

  std::string code() const
  { return Code_; }

  std::string wrappedCode() const
  { return WrappedCode_; }

private:
  using RunnerType = cppbind::CreateWrapperTestToolRunner;
  using FactoryType = decltype(std::declval<RunnerType>().makeFactory());

  static std::string wrapCode(std::string const &Code)
  {
    RunnerType Runner;

    auto Factory(Runner.makeFactory());

    cppbind::parseFundamentalTypes(Factory);

    runTool(Factory, Code);

    std::string StrSource(Runner.strSource());

    return cppbind::normalizeWhitespaceStr(StrSource);
  }

  static void runTool(FactoryType const &Factory, std::string const &Code)
  {
    clang::tooling::runToolOnCode(Factory->create(),
                                  "namespace test { " + Code + "}");
  }

  std::string Code_, WrappedCode_;
};

std::ostream &operator<<(std::ostream& OS, Snippet const &Snip)
{
  OS << Snip.code() << "\nwrapped as: \"" << Snip.wrappedCode() << "\"";
  return OS;
}

class WrappedAs : public Catch::MatcherBase<Snippet>
{
public:
  WrappedAs(std::string const &WrappedCodeExpected)
  : WrappedCodeExpected_(WrappedCodeExpected)
  {}

  bool match(Snippet const &Snip) const override
  { return Snip.wrappedCode() == WrappedCodeExpected_; }

  std::string describe() const override
  { return "\nbut should be wrapped as: \"" + WrappedCodeExpected_ + "\""; }

private:
  std::string WrappedCodeExpected_;
};

} // namespace

TEST_CASE("CPPBindTest")
{
  cppbind::Options().init();
  cppbind::Options().set<std::string>("namespace", "test");

  REQUIRE_THAT(Snippet("void foo() {}"),
               WrappedAs("void test_foo() { test::foo(); }"));
}
