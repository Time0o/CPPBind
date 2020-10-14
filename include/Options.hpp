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

#include "OptionConstants.hpp"
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

    using AssertFunc = std::function<bool(T const &)>;

  public:
    Option(llvm::StringRef Name)
    : Name_(Name)
    {}

    ~Option()
    { OptionsRegistry::instance().addOpt(*this); }

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

    Option &addAssertion(AssertFunc &&Assertion, std::string const &Msg)
    { Assertions_.emplace_back(Assertion, Msg); return *this; }

  private:
    void assertValue(T const &Value) const
    {
      for (auto const &[Assertion, Msg] : Assertions_) {
        if (!Assertion(Value))
          throw std::runtime_error(Name_.str() + ": " + Msg);
      }
    }

    llvm::StringRef Name_;

    std::optional<llvm::StringRef> Desc_;
    std::optional<llvm::StringRef> ValueDesc_;
    std::optional<bool> Optional_;
    std::optional<OptionChoices<T>> Choices_;
    std::optional<T> Default_ = std::nullopt;

    std::vector<std::pair<AssertFunc, std::string>> Assertions_;
  };

public:
  OptionsRegistry(OptionsRegistry const &)  = delete;
  OptionsRegistry(OptionsRegistry &&)       = delete;
  void operator=(OptionsRegistry const &) = delete;
  void operator=(OptionsRegistry &&)      = delete;

  template<typename T>
  Option<T> add(llvm::StringRef Name) const
  { return Option<T>(Name); }

  bool present(llvm::StringRef Name) const
  { return instance().optPresent(Name); }

  template<typename T>
  T get(llvm::StringRef Name) const
  { return *instance().findOpt<T>(Name); }

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
  void addOpt(Option<T> const &Option)
  {
    // XXX aliases
    // XXX indicate mandatory options and default values in Desc

    if (Opts_.find(Option.Name_) != Opts_.end())
      return;

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

    if (Option.Default_) {
      Option.assertValue(*Option.Default_);

      Opt->setInitialValue(*Option.Default_);

      OptsPresent_.insert(Option.Name_);
    }

    Opt->setCallback([&](T const &Value){
      Option.assertValue(Value);

      if (!Option.Default_)
        OptsPresent_.insert(Option.Name_);
    });

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

    Opts_[Option.Name_] = Opt;
  }

  bool optPresent(llvm::StringRef Name) const
  { return OptsPresent_.find(Name) != OptsPresent_.end(); }

  template<typename T>
  auto findOpt(llvm::StringRef Name) const
  {
    auto OptIt(Opts_.find(Name));

    assert(OptIt != Opts_.end() && "valid option name");
    assert(optPresent(Name) && "option present");

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

  llvm::cl::OptionCategory Category_;

  std::string Usage_;
  std::vector<llvm::cl::extrahelp> Help_;

  std::unordered_map<std::string, std::any> Opts_;
  std::unordered_set<std::string> OptsPresent_;
};

template<>
llvm::StringRef OptionsRegistry::get<llvm::StringRef>(llvm::StringRef Name) const
{
  auto Opt(instance().findOpt<std::string>(Name));
  return Opt->c_str();
}

template<>
bool OptionsRegistry::get<bool>(llvm::StringRef Name) const
{ return instance().present(Name); }

inline OptionsRegistry &Options()
{ return OptionsRegistry::instance(); }

} // namespace cppbind

#endif // GUARD_OPTIONS_H
