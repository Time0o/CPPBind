#include <regex>
#include <string>
#include <vector>

#include "Identifier.hpp"
#include "Options.hpp"
#include "String.hpp"

namespace cppbind
{

void OptionsRegistry::init()
{
  Options().add<std::string>("backend")
    .setDescription("Language for which to create bindings")
    .setOptional(false)
    .done();

  Options().add<std::vector<std::string>>("wrap-rule")
    .setDescription("Matcher rule for declarations to be wrapped")
    .setOptional(false)
    .done();

  Options().add<std::vector<std::string>>("wrap-template-instantiations")
    .setDescription("File containing extra template instantiations", "path")
    .done();

  Options().add<bool>("wrap-skip-unwrappable")
    .setDescription("Skip unwrappable objects instead of failing")
    .setDefault(false)
    .done();

  auto validPostfix = [](char const *Pat){
    return [=](std::string Postfix){
      if (string::replaceAll(Postfix, Pat, "x") == 0)
        return false;

      return Identifier::isIdentifier(Postfix) && Postfix.back() != '_';
    };
  };

  Options().add<std::string>("wrapper-func-overload-postfix")
    .setDescription("Wrapper function overload postfix, "
                    "use %o to denote #overload", "postfix")
    .setDefault("_%o")
    .addAssertion(validPostfix("%o"),
                  "postfix must create valid identifiers")
    .done();

  Options().add<std::string>("wrapper-func-numbered-param-postfix")
    .setDescription("Wrapper function numbered parameter postfix, "
                    "use %p to denote parameter number", "postfix")
    .setDefault("_%p")
    .addAssertion(validPostfix("%p"),
                  "postfix must create valid identifiers")
    .done();

  Options().add<std::string>("output-directory")
    .setDescription("Directory in which to place generated files", "path")
    .setDefault(".")
    .done();

  Options().add<std::string>("output-c-header-extension")
    .setDescription("Output C header file extension")
    .setDefault(".h")
    .done();

  Options().add<std::string>("output-c-source-extension")
    .setDescription("Output C source file extension")
    .setDefault(".c")
    .done();

  Options().add<std::string>("output-cpp-header-extension")
    .setDescription("Output C++ header file extension")
    .setDefault(".hpp")
    .done();

  Options().add<std::string>("output-cpp-source-extension")
    .setDescription("Output C++ source file extension")
    .setDefault(".cpp")
    .done();

  Options().add<int>("verbosity")
    .setDescription("Output verbosity")
    .setDefault(0)
    .addAssertion([](int Verbosity){ return Verbosity >= 0; },
                  "Verbosity must be non negative")
    .done();
}

} // namespace cppbind
