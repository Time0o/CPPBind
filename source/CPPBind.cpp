#include <iostream>

#include "CreateWrapper.hpp"
#include "Identifier.hpp"
#include "Logging.hpp"
#include "Options.hpp"
#include "String.hpp"

using namespace cppbind;

int main(int argc, char const **argv)
{
  Options().add<std::string>("namespace")
    .setDescription("Namespace in which to look for classes", "ns")
    .setOptional(false);

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
           .setDefault(CaseDefault);

  Options().add<Identifier::Case>("func-case")
           .setDescription("Wrapper function name case:", "case")
           .setChoices(CaseChoices)
           .setDefault(CaseDefault);

  Options().add<Identifier::Case>("param-case")
           .setDescription("Wrapper function parameter name case:", "case")
           .setChoices(CaseChoices)
           .setDefault(CaseDefault);

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
                         "postfix must create valid identifiers");

  Options().add<std::string>("wrapper-func-unnamed-param-placeholder")
           .setDescription("Wrapper function unnamed parameter placeholder, "
                            "use %p to denote parameter number", "placeholder")
           .setDefault("_%p")
           .addAssertion(validPostfix("%p"),
                         "postfix must create valid identifiers");

  Options().add<std::string>("wrapper-header-postfix")
           .setDescription("Output header file basename postfix", "postfix")
           .setDefault("_c");

  Options().add<std::string>("wrapper-header-ext")
           .setDescription("Output header file extension", "ext")
           .setDefault("h");

  Options().add<std::string>("wrapper-header-guard-prefix")
           .setDescription("Output header guard prefix", "prefix")
           .setDefault("GUARD_");

  Options().add<std::string>("wrapper-header-guard-postfix")
           .setDescription("Output header guard postfix", "postfix")
           .setDefault("_C_H");

  Options().add<bool>("wrapper-header-omit-extern-c")
           .setDescription("Do not use 'extern \"C\"' guards");

  Options().add<std::string>("wrapper-source-postfix")
           .setDescription("Output source file basename postfix", "postfix")
           .setDefault("_c");

  Options().add<std::string>("wrapper-source-ext")
           .setDescription("Output source file extension", "ext")
           .setDefault("cc");

  Options().add<std::string>("wrapper-header-outdir")
           .setDescription("Output header file directory", "dir")
           .setDefault(".");

  Options().add<std::string>("wrapper-source-outdir")
           .setDescription("Output source file directory", "dir")
           .setDefault(".");

  auto Parser(Options().parser(argc, argv));

  CreateWrapperToolRunner Runner;

  try {
    Runner.run(Parser);
  } catch (CPPBindError const &Err) {
    std::cerr << Err.what() << std::endl;
  }
}
