#include <iostream>

#include "CreateWrapper.hpp"
#include "Identifier.hpp"
#include "Logging.hpp"
#include "Options.hpp"
#include "OptionsInit.hpp"
#include "String.hpp"

using namespace cppbind;

int main(int argc, char const **argv)
{
  initOptions();

  auto Parser(Options().parser(argc, argv));

  CreateWrapperToolRunner Runner;

  try {
    Runner.run(Parser);
  } catch (CPPBindError const &Err) {
    std::cerr << Err.what() << std::endl;
  }
}
