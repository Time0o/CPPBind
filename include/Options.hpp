#ifndef GUARD_OPTIONS_H
#define GUARD_OPTIONS_H

#include <any>
#include <cassert>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <experimental/type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clang/Tooling/CommonOptionsParser.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"

#include "Logging.hpp"
#include "String.hpp"
#include "Mixin.hpp"

namespace cppbind
{

template<typename T>
using OptionChoices =
  std::initializer_list<std::tuple<llvm::StringRef, T, llvm::StringRef>>;

class OptionsRegistry : private mixin::NotCopyOrMoveable
{
  friend OptionsRegistry &Options();

  template<typename U>
  using value_push_back_t =
    decltype(std::declval<U>().push_back(std::declval<typename U::value_type>()));

  template<typename U>
  static inline constexpr bool has_value_push_back_v =
    std::experimental::is_detected_v<value_push_back_t, U>;

  template<typename U>
  static inline constexpr bool is_container_v =
    has_value_push_back_v<U> && !std::is_same_v<U, std::string>;

  template<typename T = std::string>
  class Option
  {
    friend OptionsRegistry;

    using Assertion = std::function<bool(T const &)>;

  public:
    explicit Option(llvm::StringRef Name)
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

    Option &setChoices(OptionChoices<T> Choices)
    { Choices_ = Choices; return *this; }

    Option &setDefault(T const &Default)
    { Default_ = Default; return *this; }

    Option &setOptional(bool Optional)
    { Optional_ = Optional; return *this; }

    Option &addAssertion(Assertion &&Assert, std::string const &Msg)
    { Assertions_.emplace_back(Assert, Msg); return *this; }

    void done() const
    { OptionsRegistry::instance().addOption(*this); }

  private:
    llvm::StringRef Name_;

    std::optional<llvm::StringRef> Desc_;
    std::optional<llvm::StringRef> ValueDesc_;
    std::optional<OptionChoices<T>> Choices_;
    std::optional<T> Default_ = std::nullopt;

    bool Optional_ = true;

    std::vector<std::pair<Assertion, std::string>> Assertions_;
  };

public:
  static void init();

  template<typename T = std::string>
  std::enable_if_t<!is_container_v<T>, T>
  get(llvm::StringRef Name) const
  {
    auto Opt_(instance().findOption(Name));

    auto Opt(std::dynamic_pointer_cast<llvm::cl::opt<T>>(Opt_));
    if (!Opt)
      throw log::exception("{0}: invalid type", Name);

    T Value = *Opt;

    assertOptionValue(Name, Value);

    return Value;
  }

  template<typename T>
  std::enable_if_t<is_container_v<T>, T>
  get(llvm::StringRef Name) const
  {
    using V = typename T::value_type;

    auto Opt_(instance().findOption(Name));

    auto Opt(std::dynamic_pointer_cast<llvm::cl::list<V>>(Opt_));
    if (!Opt)
      throw log::exception("{0}: invalid type", Name);

    T Values;
    for (auto const &Value : *Opt)
      Values.push_back(Value);

    assertOptionValue(Name, Values);

    return Values;
  }

  clang::tooling::CommonOptionsParser parser(int Argc, char const **Argv)
  { return {Argc, Argv, Category_}; }

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
  void addOption(Option<T> Option)
  {
    // XXX aliases
    // XXX indicate mandatory options and default values in Desc

    auto Opt(createOption<T>(Option.Name_, llvm::cl::cat(Category_)));

    Opts_[Option.Name_.str()] = Opt;

    if (Option.Desc_)
      Opt->setDescription(*Option.Desc_);

    if (Option.ValueDesc_)
      Opt->setValueStr(*Option.ValueDesc_);

    if constexpr (is_container_v<T>) {
      Opt->setNumOccurrencesFlag(Option.Optional_ ? llvm::cl::Optional
                                                  : llvm::cl::OneOrMore);

    } else {
      Opt->setNumOccurrencesFlag(Option.Optional_ ? llvm::cl::Optional
                                                  : llvm::cl::Required);

      if constexpr (std::is_same_v<T, bool>) {
        Opt->setValueExpectedFlag(Option.Optional_ ? llvm::cl::ValueOptional
                                                   : llvm::cl::ValueRequired);
      }

      auto OptSingleValued(std::dynamic_pointer_cast<llvm::cl::opt<T>>(Opt));

      if (Option.Choices_) {
        if constexpr (std::is_enum_v<T>) {
          for (auto const &Choice : *Option.Choices_) {
            OptSingleValued->getParser().addLiteralOption(std::get<0>(Choice),
                                                          std::get<1>(Choice),
                                                          std::get<2>(Choice));
          }
        } else {
          assert(false && "multi-choice options must be of enum type");
        }
      }

      if (Option.Default_)
        OptSingleValued->setInitialValue(*Option.Default_);
    }

    if (!Option.Assertions_.empty())
      OptAssertions_[Option.Name_.str()] = Option.Assertions_;
  }

  template<typename T, typename ...ARGS>
  std::enable_if_t<!is_container_v<T>, std::shared_ptr<llvm::cl::Option>>
  createOption(llvm::StringRef Name, ARGS&&... Args)
  {
    auto Opt(std::make_shared<llvm::cl::opt<T>>(Name,
                                                std::forward<ARGS>(Args)...));

    Opts_[Name.str()] = Opt;

    return Opt;
  }

  template<typename T, typename ...ARGS>
  std::enable_if_t<is_container_v<T>, std::shared_ptr<llvm::cl::Option>>
  createOption(llvm::StringRef Name, ARGS&&... Args)
  {
    using V = typename T::value_type;

    auto Opt(std::make_shared<llvm::cl::list<V>>(Name,
                                                 std::forward<ARGS>(Args)...));

    return Opt;
  }

  std::shared_ptr<llvm::cl::Option> findOption(llvm::StringRef Name) const
  {
    auto OptIt(Opts_.find(Name.str()));

    assert(OptIt != Opts_.end() && "valid option name");

    return OptIt->second;
  }

  template<typename T>
  void assertOptionValue(llvm::StringRef Name, T const &Value) const
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
        throw log::exception("{0}: {1}", Name, Msg);
    }
  }

  llvm::cl::OptionCategory Category_;

  std::string Usage_;
  std::vector<llvm::cl::extrahelp> Help_;

  std::unordered_map<std::string, std::shared_ptr<llvm::cl::Option>> Opts_;
  std::unordered_map<std::string, std::any> OptAssertions_;
};

inline OptionsRegistry &Options()
{ return OptionsRegistry::instance(); }

} // namespace cppbind

#define OPT1(name)       Options().get(name)
#define OPT2(type, name) Options().get<type>(name)

#define OPT_MUX(_1, _2, MACRO, ...) MACRO
#define OPT(...) OPT_MUX(__VA_ARGS__, OPT2, OPT1, _)(__VA_ARGS__)

#endif // GUARD_OPTIONS_H
