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

  auto validMatchers = [](std::vector<std::string> const &Matchers) {
    std::regex MatcherRegex(R"#([a-zA-Z]+:[a-zA-Z\(\)"]+)#");

    for (auto const &Matcher : Matchers) {
      if (!std::regex_match(Matcher, MatcherRegex))
        return false;
    }

    return true;
  };

  Options().add<std::vector<std::string>>("match")
    .setDescription("Matcher rules for declarations to be wrapped", "match")
    .setOptional(false)
    .addAssertion(validMatchers, "invalid matcher rule (must have form decl_type:matcher)")
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
