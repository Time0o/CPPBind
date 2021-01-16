#ifndef GUARD_OPTIONS_INIT_H
#define GUARD_OPTIONS_INIT_H

#include <string>

#include "Backend.hpp"
#include "Identifier.hpp"

namespace cppbind
{

inline void initOptions()
{
  Options().add<std::string>("backend")
    .setDescription("Language for which to create bindings", "be")
    .setOptional(false)
    .done();

  Options().add<std::string>("namespace")
    .setDescription("Namespace in which to look for classes", "ns")
    .setOptional(false)
    .done();

  Options().add<std::string>("output-directory")
    .setDescription("Directory in which to place generated files", "ns")
    .setDefault("")
    .done();

  // XXX remove below

  Options().add<bool>("overload-default-params")
    .setDescription("Create overloads for functions with default parameters")
    .setDefault(false)
    .done();

  OptionChoices<Identifier::Case> CaseChoices{
    {"camel-case", Identifier::CAMEL_CASE, "camelCase"},
    {"pascal-case", Identifier::PASCAL_CASE, "PascalCase"},
    {"snake-case", Identifier::SNAKE_CASE, "snake_case"},
    {"snake-case-cap-first", Identifier::SNAKE_CASE_CAP_FIRST, "Snake_case"},
    {"snake-case-cap-all", Identifier::SNAKE_CASE_CAP_ALL, "SNAKE_CASE"}
  };

  Identifier::Case CaseDefault = Identifier::SNAKE_CASE;

  Options().add<Identifier::Case>("type-case")
    .setDescription("Wrapper type name case:", "case")
    .setChoices(CaseChoices)
    .setDefault(CaseDefault)
    .done();

  Options().add<Identifier::Case>("func-case")
    .setDescription("Wrapper function name case:", "case")
    .setChoices(CaseChoices)
    .setDefault(CaseDefault)
    .done();

  Options().add<Identifier::Case>("param-case")
    .setDescription("Wrapper function parameter name case:", "case")
    .setChoices(CaseChoices)
    .setDefault(CaseDefault)
    .done();

  auto validPostfix = [](char const *Pat){
    return [=](std::string Postfix){
      if (replaceAllStrs(Postfix, Pat, "x") == 0)
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

  Options().add<std::string>("wrapper-header-postfix")
    .setDescription("Output header file basename postfix", "postfix")
    .setDefault("_c")
    .done();

  Options().add<std::string>("wrapper-header-ext")
    .setDescription("Output header file extension", "ext")
    .setDefault("h")
    .done();

  Options().add<std::string>("wrapper-header-guard-prefix")
    .setDescription("Output header guard prefix", "prefix")
    .setDefault("GUARD_")
    .done();

  Options().add<std::string>("wrapper-header-guard-postfix")
    .setDescription("Output header guard postfix", "postfix")
    .setDefault("_C_H")
    .done();

  Options().add<bool>("wrapper-header-omit-extern-c")
    .setDescription("Do not use 'extern \"C\"' guards")
    .setDefault(false)
    .done();

  Options().add<std::string>("wrapper-source-postfix")
    .setDescription("Output source file basename postfix", "postfix")
    .setDefault("_c")
    .done();

  Options().add<std::string>("wrapper-source-ext")
    .setDescription("Output source file extension", "ext")
    .setDefault("cc")
    .done();

  Options().add<std::string>("wrapper-header-outdir")
    .setDescription("Output header file directory", "dir")
    .setDefault(".")
    .done();

  Options().add<std::string>("wrapper-source-outdir")
    .setDescription("Output source file directory", "dir")
    .setDefault(".")
    .done();
}

} // namespace cppbind

#endif // GUARD_OPTIONS_INIT_H
