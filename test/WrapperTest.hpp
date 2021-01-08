#include <memory>
#include <string>

#include "llvm/ADT/StringRef.h"

#include "catch2/catch.hpp"

#include "OptionsInit.hpp"
#include "Options.hpp"
#include "Wrappable.hpp"
#include "WrappedBy.hpp"

namespace cppbind_test
{

void WrapperTest(std::string const &What,
                 Wrappable const &CXXToWrap,
                 WrappedBy const &CWrappedBy)
{
  INFO(What);
  CHECK_THAT(CXXToWrap.done(), CWrappedBy.done());
}

inline int wrapperTestMain(int argc, char **argv)
{
  cppbind::initOptions();

  cppbind::Options().set<std::string>("namespace", "test");

  return Catch::Session().run(argc, argv);
}

} // namespace cppbind_test
