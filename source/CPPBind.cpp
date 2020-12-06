#include <iostream>

#include "CreateWrapper.hpp"
#include "Identifier.hpp"
#include "Logging.hpp"
#include "Options.hpp"
#include "String.hpp"

using namespace cppbind;

int main(int argc, char const **argv)
{
  Options().init();

  auto Parser(Options().parser(argc, argv));

  CreateAndWriteWrapperToolRunner Runner;

  try {
    Runner.run(Parser);
  } catch (CPPBindError const &Err) {
    std::cerr << Err.what() << std::endl;
  }
}
