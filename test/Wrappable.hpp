#ifndef GUARD_WRAPPABLE_H
#define GUARD_WRAPPABLE_H

#include <memory>
#include <set>
#include <sstream>
#include <string>

#include "CreateWrapper.hpp"
#include "String.hpp"
#include "WrapperInclude.hpp"

namespace cppbind_test
{

class Wrappable__
{
  friend class Wrappable;

public:
  std::string getWrapperCode(bool Source)
  {
    if (Source) {
      DebugSource_ = true;
      return WrapperSource_;
    } else {
      DebugSource_ = false;
      return WrapperHeader_;
    }
  }

  std::set<cppbind::WrapperInclude> getWrapperIncludes(bool Source)
  {
    DebugIncludes_ = true;

    return Source ? WrapperSourceIncludes_ : WrapperHeaderIncludes_;
  }

  std::string describe()
  {
    std::ostringstream SS;
    SS << Code_ << '\n'
       << ">>> wrapped as:" << '\n'
       << getWrapperCode(DebugSource_);

    if (DebugIncludes_) {
       SS << '\n' << ">>> with includes:";

       for (auto const &Include :
            (DebugSource_ ? WrapperSourceIncludes_ : WrapperHeaderIncludes_))
         SS << '\n' << Include;
    }

    return SS.str();
  }

private:
  Wrappable__(std::string const &Code)
  : Code_(Code)
  {
    cppbind::CreateWrapperTestToolRunner Runner;

    Runner.run("namespace test {" + Code + "}");

    WrapperHeader_ = Runner.strHeader();
    WrapperSource_ = Runner.strSource();

    cppbind::normalizeWhitespaceStr(WrapperHeader_);
    cppbind::normalizeWhitespaceStr(WrapperSource_);

    for (auto const &Include : Runner.includes()) {
      if (Include.isPublic())
        WrapperHeaderIncludes_.insert(Include);

      WrapperSourceIncludes_.insert(Include);
    }
  }

  std::string Code_;

  std::string WrapperHeader_;
  std::string WrapperSource_;

  std::set<cppbind::WrapperInclude> WrapperHeaderIncludes_;
  std::set<cppbind::WrapperInclude> WrapperSourceIncludes_;

  bool DebugSource_ = false;
  bool DebugIncludes_ = false;
};

using Wrappable_ = std::shared_ptr<Wrappable__>;

class Wrappable
{
public:
  Wrappable(std::string const &Code)
  : Wr_(Code)
  {}

  Wrappable_ done() const
  { return std::make_shared<Wrappable__>(Wr_); }

private:
  Wrappable__ Wr_;
};

} // namespace cppbind_test

namespace Catch
{
  template<>
  struct StringMaker<cppbind_test::Wrappable_>
  {
    static std::string convert(cppbind_test::Wrappable_ const &WP)
    { return WP->describe(); }
  };
} // namespace Catch

#endif // GUARD_WRAPPABLE_H
