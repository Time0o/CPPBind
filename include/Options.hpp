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
  friend void initOptions();
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

  template<typename T = std::string>
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
  : Category_(Category),
    Usage_(string::indent(string::trim(Usage.str())))
  {
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
      Assertions = std::any_cast<decltype(Assertions)>(It->second);
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
inline llvm::StringRef OptionsRegistry::get<llvm::StringRef>(
  llvm::StringRef Name) const
{
  auto Opt(instance().findOpt<std::string>(Name));
  return Opt->c_str();
}

inline OptionsRegistry &Options()
{ return OptionsRegistry::instance(); }

} // namespace cppbind

#endif // GUARD_OPTIONS_H
