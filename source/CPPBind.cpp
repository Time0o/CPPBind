#include <cstdlib>
#include <exception>

#include "CreateWrapper.hpp"
#include "Logging.hpp"
#include "Options.hpp"

using namespace cppbind;

int main(int argc, char const **argv)
{
  Options().init();

  auto Parser(Options().parser(argc, argv));

  CreateWrapperToolRunner Runner(Parser);

  log::Verbosity = OPT(int, "verbosity");

  try {
    Runner.run();
  } catch (std::exception const &Err) {
    log::error(Err.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
