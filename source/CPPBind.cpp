#include <cstdlib>

#include "CreateWrapper.hpp"
#include "Error.hpp"
#include "Options.hpp"
#include "OptionsInit.hpp"

using namespace cppbind;

int main(int argc, char const **argv)
{
  initOptions();

  auto Parser(Options().parser(argc, argv));

  CreateWrapperToolRunner Runner;

  try {
    Runner.run(Parser);
  } catch (CPPBindError const &Err) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
