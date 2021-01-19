#include <cstdlib>

#include "CreateWrapper.hpp"
#include "Error.hpp"
#include "Logging.hpp"
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
    log::error() << Err.what();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
