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
    .setDescription("Language for which to create bindings", "be")
    .setOptional(false)
    .done();

  Options().add<std::vector<std::string>>("wrap")
    .setDescription("Matcher rules for declarations to be wrapped", "wrap")
    .setOptional(false)
    .done();

  Options().add<bool>("skip-unwrappable")
    .setDescription("Skip unwrappable objects instead of failing")
    .setDefault(false)
    .done();

  Options().add<std::string>("output-directory")
    .setDescription("Directory in which to place generated files", "outdir")
    .setDefault("")
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

  Options().add<bool>("verbose")
    .setDescription("More verbose output")
    .done();
}

} // namespace cppbind
