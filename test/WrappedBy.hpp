#ifndef GUARD_WRAPPED_BY_H
#define GUARD_WRAPPED_BY_H

#include <set>
#include <sstream>
#include <string>
#include <utility>

#include "catch2/catch.hpp"

#include "String.hpp"
#include "WrapperInclude.hpp"

namespace cppbind_test
{

class WrappedBy_ : public Catch::MatcherBase<Wrappable_>
{
  friend class WrappedBy;

public:
  bool match(Wrappable_ const &WP) const override
  { return codeMatches(WP) && includesMatch(WP); }

  std::string describe() const override
  {
    std::ostringstream SS;
    SS << '\n' << ">>> but should be wrapped as:"
       << '\n' << CodeExpected_;

    if (!IncludesExpected_.empty()) {
      SS << '\n' << ">>> and include:";
      for (auto const &Include : IncludesExpected_)
        SS << '\n' << Include;
    }

    return SS.str();
  }

private:
  WrappedBy_(std::string const &CodeExpected)
  : CodeExpected_(CodeExpected)
  { cppbind::normalizeWhitespaceStr(CodeExpected_); }

  bool codeMatches(Wrappable_ const &WP) const
  { return codeActual(WP) == CodeExpected_; }

  std::string codeActual(Wrappable_ const &WP) const
  { return WP->getWrapperCode(source()); }

  bool includesMatch(Wrappable_ const &WP) const
  {
    if (IncludesExpected_.empty())
      return true;

    auto IncludesActual(includesActual(WP));

    return std::includes(IncludesActual.begin(), IncludesActual.end(),
                         IncludesExpected_.begin(), IncludesExpected_.end());
  }

  std::set<cppbind::WrapperInclude> includesActual(Wrappable_ const &WP) const
  { return WP->getWrapperIncludes(source()); }

  bool source() const
  { return CodeExpected_.find("test::") != std::string::npos; }

  std::string CodeExpected_;
  std::set<cppbind::WrapperInclude> IncludesExpected_;
};

class WrappedBy
{
public:
  WrappedBy(std::string const &CodeExpected)
  : Wb_(CodeExpected)
  {}

  template<typename ...ARGS>
  WrappedBy &mustInclude(std::string const &Header, bool System)
  {
    Wb_.IncludesExpected_.emplace(Header, System);
    return *this;
  }

  WrappedBy_ done() const
  { return Wb_; }

private:
  WrappedBy_ Wb_;
};

} // namespace cppbind_test

#endif // GUARD_WRAPPED_BY_H
