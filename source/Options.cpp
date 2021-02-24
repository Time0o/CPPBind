#include <string>

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

  Options().add<std::string>("namespace")
    .setDescription("Namespace in which to look for bindable entities", "ns")
    .setOptional(false)
    .done();

  Options().add<bool>("ignore-nested-namespaces")
    .setDescription("Don't look for bindable entities in nested namespaces")
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

  Options().add<std::string>("wrapper-func-unnamed-param-placeholder")
    .setDescription("Wrapper function unnamed parameter placeholder, "
                     "use %p to denote parameter number", "placeholder")
    .setDefault("_%p")
    .addAssertion(validPostfix("%p"),
                  "postfix must create valid identifiers")
    .done();
}

} // namespace cppbind