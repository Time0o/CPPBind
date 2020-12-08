#ifndef GUARD_OPTIONS_H
#define GUARD_OPTIONS_H

#include <any>
#include <cassert>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "clang/Driver/Options.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Option/OptTable.h"
#include "llvm/Support/CommandLine.h"

#include "Logging.hpp"
#include "String.hpp"

namespace cppbind
{

template<typename T>
using OptionChoices =
  std::initializer_list<std::tuple<llvm::StringRef, T, llvm::StringRef>>;

class OptionsRegistry
{
  friend OptionsRegistry &Options();

  template<typename T = std::string>
  class Option
  {
    friend OptionsRegistry;

    using Assertion = std::function<bool(T const &)>;

  public:
    Option(llvm::StringRef Name)
    : Name_(Name)
    {}

    Option &setDescription(llvm::StringRef Desc,
                           llvm::StringRef ValueDesc = "")
    {
      Desc_ = Desc;

      if (!ValueDesc.empty())
          ValueDesc_ = ValueDesc;

      return *this;
    }

    Option &setOptional(bool Optional)
    { Optional_ = Optional; return *this; }

    Option &setChoices(OptionChoices<T> Choices)
    { Choices_ = Choices; return *this; }

    Option &setDefault(T const &Default)
    { Default_ = Default; return *this; }

    Option &addAssertion(Assertion &&Assert, std::string const &Msg)
    { Assertions_.emplace_back(Assert, Msg); return *this; }

    void done() const
    { OptionsRegistry::instance().addOpt(*this); }

  private:
    llvm::StringRef Name_;

    std::optional<llvm::StringRef> Desc_;
    std::optional<llvm::StringRef> ValueDesc_;
    std::optional<bool> Optional_;
    std::optional<OptionChoices<T>> Choices_;
    std::optional<T> Default_ = std::nullopt;

    std::vector<std::pair<Assertion, std::string>> Assertions_;
  };

public:
  OptionsRegistry(OptionsRegistry const &)  = delete;
  OptionsRegistry(OptionsRegistry &&)       = delete;
  void operator=(OptionsRegistry const &) = delete;
  void operator=(OptionsRegistry &&)      = delete;

  void init()
  {
    add<std::string>("namespace")
      .setDescription("Namespace in which to look for classes", "ns")
      .setOptional(false)
      .done();

    OptionChoices<Identifier::Case> CaseChoices{
      {"camel-case", Identifier::CAMEL_CASE, "camelCase"},
      {"pascal-case", Identifier::PASCAL_CASE, "PascalCase"},
      {"snake-case", Identifier::SNAKE_CASE, "snake_case"},
      {"snake-case-cap-first", Identifier::SNAKE_CASE_CAP_FIRST, "Snake_case"},
      {"snake-case-cap-all", Identifier::SNAKE_CASE_CAP_ALL, "SNAKE_CASE"}
    };

    Identifier::Case CaseDefault = Identifier::SNAKE_CASE;

    add<Identifier::Case>("type-case")
      .setDescription("Wrapper type name case:", "case")
      .setChoices(CaseChoices)
      .setDefault(CaseDefault)
      .done();

    add<Identifier::Case>("func-case")
      .setDescription("Wrapper function name case:", "case")
      .setChoices(CaseChoices)
      .setDefault(CaseDefault)
      .done();

    add<Identifier::Case>("param-case")
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

    add<std::string>("wrapper-func-overload-postfix")
      .setDescription("Wrapper function overload postfix, "
                      "use %o to denote #overload", "postfix")
      .setDefault("_%o")
      .addAssertion(validPostfix("%o"),
                    "postfix must create valid identifiers")
      .done();

    add<std::string>("wrapper-func-unnamed-param-placeholder")
      .setDescription("Wrapper function unnamed parameter placeholder, "
                       "use %p to denote parameter number", "placeholder")
      .setDefault("_%p")
      .addAssertion(validPostfix("%p"),
                    "postfix must create valid identifiers")
      .done();

    add<std::string>("wrapper-header-postfix")
      .setDescription("Output header file basename postfix", "postfix")
      .setDefault("_c")
      .done();

    add<std::string>("wrapper-header-ext")
      .setDescription("Output header file extension", "ext")
      .setDefault("h")
      .done();

    add<std::string>("wrapper-header-guard-prefix")
      .setDescription("Output header guard prefix", "prefix")
      .setDefault("GUARD_")
      .done();

    add<std::string>("wrapper-header-guard-postfix")
      .setDescription("Output header guard postfix", "postfix")
      .setDefault("_C_H")
      .done();

    add<bool>("wrapper-header-omit-extern-c")
      .setDescription("Do not use 'extern \"C\"' guards")
      .setDefault(false)
      .done();

    add<std::string>("wrapper-source-postfix")
      .setDescription("Output source file basename postfix", "postfix")
      .setDefault("_c")
      .done();

    add<std::string>("wrapper-source-ext")
      .setDescription("Output source file extension", "ext")
      .setDefault("cc")
      .done();

    add<std::string>("wrapper-header-outdir")
      .setDescription("Output header file directory", "dir")
      .setDefault(".")
      .done();

    add<std::string>("wrapper-source-outdir")
      .setDescription("Output source file directory", "dir")
      .setDefault(".")
      .done();
  }

  template<typename T>
  T get(llvm::StringRef Name) const
  {
    T Value = *instance().findOpt<T>(Name);

    assertOptValue(Name, Value);

    return Value;
  }

  template<typename T>
  void set(llvm::StringRef Name, T const &Value) const
  { *findOpt<T>(Name) = Value; }

  clang::tooling::CommonOptionsParser parser(int Argc, char const **Argv)
  { return clang::tooling::CommonOptionsParser(Argc, Argv, Category_); }

private:
  OptionsRegistry(llvm::StringRef Category, llvm::StringRef Usage)
  : Category_(Category)
  {
    Usage_ = Usage.str();
    Usage_ = indentStr(trimStr(Usage_));

    Help_.emplace_back(clang::tooling::CommonOptionsParser::HelpMessage);
    Help_.emplace_back(Usage_.c_str());
  }

  static OptionsRegistry &instance()
  {
    static OptionsRegistry OP("Cppbind options",
                              "Generate C bindings to C++ classes");
    return OP;
  }

  template<typename T>
  Option<T> add(llvm::StringRef Name) const
  { return Option<T>(Name); }

  template<typename T>
  void addOpt(Option<T> Option)
  {
    // XXX aliases
    // XXX indicate mandatory options and default values in Desc

    assert(Opts_.find(Option.Name_.str()) == Opts_.end());

    auto Opt(packOpt<T>(Option.Name_, llvm::cl::cat(Category_)));

    if (Option.Desc_)
      Opt->setDescription(*Option.Desc_);

    if (Option.ValueDesc_)
      Opt->setValueStr(*Option.ValueDesc_);

    if (Option.Optional_)
      Opt->setNumOccurrencesFlag(*Option.Optional_ ? llvm::cl::Optional
                                                   : llvm::cl::Required);

    if constexpr (std::is_same_v<T, bool>)
      Opt->setValueExpectedFlag(*Option.Optional_ ? llvm::cl::ValueOptional
                                                  : llvm::cl::ValueRequired);

    if (Option.Default_)
      Opt->setInitialValue(*Option.Default_);

    if (Option.Choices_) {
      if constexpr (std::is_enum_v<T>) {
        for (auto const &Choice : *Option.Choices_) {
          Opt->getParser().addLiteralOption(std::get<0>(Choice),
                                            std::get<1>(Choice),
                                            std::get<2>(Choice));
        }
      } else {
        assert(false && "multi-choice options must be of enum type");
      }
    }

    Opts_[Option.Name_.str()] = Opt;

    if (!Option.Assertions_.empty())
      OptAssertions_[Option.Name_.str()] = Option.Assertions_;
  }

  template<typename T>
  auto findOpt(llvm::StringRef Name) const
  {
    auto OptIt(Opts_.find(Name.str()));

    assert(OptIt != Opts_.end() && "valid option name");

    return unpackOpt<T>(OptIt->second);
  }

  template<typename T, typename ...ARGS>
  static auto packOpt(ARGS&&... Args)
  { return std::make_shared<llvm::cl::opt<T>>(std::forward<ARGS>(Args)...); }

  template<typename T>
  static auto unpackOpt(std::any const &Opt)
  {
    try {
      return std::any_cast<std::shared_ptr<llvm::cl::opt<T>>>(Opt);
    } catch (std::bad_any_cast const &) {
      assert(false && "valid option type");
    }
  }

  template<typename T>
  void assertOptValue(llvm::StringRef Name, T const &Value) const
  {
    auto It(OptAssertions_.find(Name.str()));
    if (It == OptAssertions_.end())
      return;

    decltype(Option<T>::Assertions_) Assertions;

    try {
      Assertions = std::any_cast<decltype(Assertions)>(*It);
    } catch (std::bad_any_cast const &) {
      assert(false && "valid option assertions");
    }

    for (auto const &[Assert, Msg] : Assertions) {
      if (!Assert(Value))
        error() << Name.str() << ": " << Msg;
    }
  }

  llvm::cl::OptionCategory Category_;

  std::string Usage_;
  std::vector<llvm::cl::extrahelp> Help_;

  std::unordered_map<std::string, std::any> Opts_;
  std::unordered_map<std::string, std::any> OptAssertions_;
};

template<>
llvm::StringRef OptionsRegistry::get<llvm::StringRef>(llvm::StringRef Name) const
{
  auto Opt(instance().findOpt<std::string>(Name));
  return Opt->c_str();
}

inline OptionsRegistry &Options()
{ return OptionsRegistry::instance(); }

} // namespace cppbind

#define OPT1(name)       Options().get<std::string>(name)
#define OPT2(type, name) Options().get<type>(name)

#define MUX_MACRO(_1, _2, MACRO, ...) MACRO
#define OPT(...) MUX_MACRO(__VA_ARGS__, OPT2, OPT1, _)(__VA_ARGS__)

#define NAMESPACE                              OPT("namespace")
#define TYPE_CASE                              OPT(Identifier::Case, "type-case")
#define FUNC_CASE                              OPT(Identifier::Case, "func-case")
#define PARAM_CASE                             OPT(Identifier::Case, "param-case")
#define WRAPPER_FUNC_OVERLOAD_POSTFIX          OPT("wrapper-func-overload-postfix")
#define WRAPPER_FUNC_UNNAMED_PARAM_PLACEHOLDER OPT("wrapper-func-unnamed-param-placeholder")
#define WRAPPER_HEADER_POSTFIX                 OPT("wrapper-header-postfix")
#define WRAPPER_HEADER_EXT                     OPT("wrapper-header-ext")
#define WRAPPER_HEADER_GUARD_PREFIX            OPT("wrapper-header-guard-prefix")
#define WRAPPER_HEADER_GUARD_POSTFIX           OPT("wrapper-header-guard-postfix")
#define WRAPPER_HEADER_OMIT_EXTERN_C           OPT(bool, "wrapper-header-omit-extern-c")
#define WRAPPER_SOURCE_POSTFIX                 OPT("wrapper-source-postfix")
#define WRAPPER_SOURCE_EXT                     OPT("wrapper-source-ext")
#define WRAPPER_HEADER_OUTDIR                  OPT("wrapper-header-outdir")
#define WRAPPER_SOURCE_OUTDIR                  OPT("wrapper-source-outdir")

#endif // GUARD_OPTIONS_H
