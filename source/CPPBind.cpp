#include <cstdlib>
#include <exception>

#include "llvm/Support/raw_ostream.h"

#include "CreateWrapper.hpp"
#include "Options.hpp"

using namespace cppbind;

int main(int argc, char const **argv)
{
  Options().init();

  auto Parser(Options().parser(argc, argv));

  CreateWrapperToolRunner Runner(Parser);

  try {
    Runner.run();
  } catch (std::exception const &Err) {
    llvm::errs() << Err.what();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
