#include <cstdlib>

#include "CreateWrapper.hpp"
#include "Error.hpp"
#include "Logging.hpp"
#include "Options.hpp"

using namespace cppbind;

int main(int argc, char const **argv)
{
  Options().init();

  auto Parser(Options().parser(argc, argv));

  CreateWrapperToolRunner Runner(Parser);

  try {
    Runner.run();
  } catch (CPPBindError const &Err) {
    log::error() << Err.what();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
