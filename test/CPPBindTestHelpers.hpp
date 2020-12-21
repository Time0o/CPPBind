#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "catch2/catch.hpp"

#include "CreateWrapper.hpp"
#include "Options.hpp"
#include "String.hpp"

#define TEST_NAMESPACE "test"

namespace cppbind_test
{

class Wrappable;
class WrappedBy;

using WrappablePtr = std::shared_ptr<Wrappable>;
using WrapperIncludes = std::vector<std::string>;

class Wrappable
{
public:
  Wrappable(std::string const &Code)
  : Code_(Code)
  {
    RunnerType Runner;

    Runner.run("namespace " TEST_NAMESPACE " {" + Code + "}");

    WrapperHeader_ = Runner.strHeader();
    WrapperSource_ = Runner.strSource();

    cppbind::normalizeWhitespaceStr(WrapperHeader_);
    cppbind::normalizeWhitespaceStr(WrapperSource_);

    WrapperIncludes_ =  Runner.includes();

    std::sort(WrapperIncludes_.begin(), WrapperIncludes_.end());
  }

  std::string getCode() const
  { return Code_; }

  std::string getWrapperCode(bool WithDefinitions)
  {
    if (WithDefinitions) {
      DebugDefinitions_ = true;
      return WrapperSource_;
    } else {
      DebugDefinitions_ = false;
      return WrapperHeader_;
    }
  }

  std::vector<std::string> getWrapperIncludes()
  {
    DebugIncludes_ = true;
    return WrapperIncludes_;
  }

  std::string describe()
  {
    std::ostringstream SS;
    SS << getCode() << '\n'
       << ">>> wrapped as:" << '\n'
       << getWrapperCode(DebugDefinitions_);

    if (DebugIncludes_) {
       SS << '\n' << ">>> with includes:";

       for (auto const &Include : WrapperIncludes_)
         SS << '\n' << Include;
    }

    return SS.str();
  }

private:
  using RunnerType = cppbind::CreateWrapperTestToolRunner;
  using FactoryType = decltype(std::declval<RunnerType>().makeFactory());

  std::string Code_;
  std::string WrapperHeader_;
  std::string WrapperSource_;
  std::vector<std::string> WrapperIncludes_;

  bool DebugDefinitions_ = false;
  bool DebugIncludes_ = false;
};

inline WrappablePtr makeWrappable(std::string const &Code)
{ return std::make_shared<Wrappable>(Code); }

class WrappedBy : public Catch::MatcherBase<WrappablePtr>
{
public:
  WrappedBy(std::string const &WrapperCodeExpected,
            WrapperIncludes const &WrapperIncludesExpected = {})
  : WrapperCodeExpected_(WrapperCodeExpected),
    WrapperCodeHasDefinitions_(
      WrapperCodeExpected_.find(TEST_NAMESPACE "::") != std::string::npos),
    WrapperIncludesExpected_(WrapperIncludesExpected)
  {
    cppbind::normalizeWhitespaceStr(WrapperCodeExpected_);

    std::sort(WrapperIncludesExpected_.begin(), WrapperIncludesExpected_.end());
  }

  bool match(WrappablePtr const &WP) const override
  {
    auto WrapperCodeActual(WP->getWrapperCode(WrapperCodeHasDefinitions_));

    bool codeMatches = WrapperCodeActual == WrapperCodeExpected_;

    if (WrapperIncludesExpected_.empty())
      return codeMatches;

    auto WrapperIncludesActual(WP->getWrapperIncludes());

    bool includesMatch = std::includes(
      WrapperIncludesActual.begin(), WrapperIncludesActual.end(),
      WrapperIncludesExpected_.begin(), WrapperIncludesExpected_.end());

    return codeMatches && includesMatch;
  }

  std::string describe() const override
  {
    std::ostringstream SS;
    SS << '\n' << ">>> but should be wrapped as:"
       << '\n' << WrapperCodeExpected_;

    if (!WrapperIncludesExpected_.empty()) {
      SS << '\n' << ">>> and include:";
      for (auto const &Include : WrapperIncludesExpected_)
        SS << '\n' << Include;
    }

    return SS.str();
  }

private:
  std::string WrapperCodeExpected_;
  bool WrapperCodeHasDefinitions_;

  std::vector<std::string> WrapperIncludesExpected_;
};

void WrapperTest(std::string const &What,
                 WrappablePtr const &CXXToWrap,
                 WrappedBy const &CWrappedBy)
{
  INFO(What);
  CHECK_THAT(CXXToWrap, CWrappedBy);
}

int wrapperTestMain(int argc, char **argv)
{
  cppbind::Options().init();
  cppbind::Options().set<std::string>("namespace", TEST_NAMESPACE);

  return Catch::Session().run(argc, argv);
}

} // namespace cppbind_test

namespace Catch
{
  template<>
  struct StringMaker<cppbind_test::WrappablePtr>
  {
    static std::string convert(cppbind_test::WrappablePtr const &WP)
    { return WP->describe(); }
  };
} // namespace Catch
