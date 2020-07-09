#ifndef GUARD_OPTIONS_H
#define GUARD_OPTIONS_H

#include <any>
#include <cassert>
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

  public:
    Option(llvm::StringRef Name)
    : _Name(Name)
    {}

    ~Option()
    { OptionsRegistry::instance().addOpt(*this); }

    Option &setDescription(llvm::StringRef Desc,
                           llvm::StringRef ValueDesc = "")
    {
      _Desc = Desc;

      if (!ValueDesc.empty())
          _ValueDesc = ValueDesc;

      return *this;
    }

    Option &setOptional(bool Optional)
    { _Optional = Optional; return *this; }

    Option &setChoices(OptionChoices<T> Choices)
    { _Choices = Choices; return *this; }

    Option &setDefault(T const &Default)
    { _Default = Default; return *this; }

  private:
    llvm::StringRef _Name;

    std::optional<llvm::StringRef> _Desc;
    std::optional<llvm::StringRef> _ValueDesc;
    std::optional<bool> _Optional;
    std::optional<OptionChoices<T>> _Choices;
    std::optional<T> _Default = std::nullopt;
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
  { return clang::tooling::CommonOptionsParser(Argc, Argv, _Category); }

private:
  OptionsRegistry(llvm::StringRef Category, llvm::StringRef Usage)
  : _Category(Category)
  {
    _Usage = Usage.str();
    _Usage = indentStr(trimStr(_Usage));

    _Help.emplace_back(clang::tooling::CommonOptionsParser::HelpMessage);
    _Help.emplace_back(_Usage.c_str());
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

    if (_Opts.find(Option._Name) != _Opts.end())
      return;

    auto Opt(packOpt<T>(Option._Name, llvm::cl::cat(_Category)));

    if (Option._Desc)
      Opt->setDescription(*Option._Desc);

    if (Option._ValueDesc)
      Opt->setValueStr(*Option._ValueDesc);

    if (Option._Optional)
      Opt->setNumOccurrencesFlag(*Option._Optional ? llvm::cl::Optional
                                                   : llvm::cl::Required);

    if constexpr (std::is_same_v<T, bool>)
      Opt->setValueExpectedFlag(*Option._Optional ? llvm::cl::ValueOptional
                                                  : llvm::cl::ValueRequired);

    if (Option._Default) {
      Opt->setInitialValue(*Option._Default);

      _OptsPresent.insert(Option._Name);

    } else {
      Opt->setCallback([&](
        typename llvm::cl::parser<T>::parser_data_type const &)
      { _OptsPresent.insert(Option._Name); });
    }

    if (Option._Choices) {
      if constexpr (!std::is_integral_v<T> &&
                    !std::is_same_v<T, std::string>)
      {
        for (auto const &Choice : *Option._Choices) {
          Opt->getParser().addLiteralOption(std::get<0>(Choice),
                                            std::get<1>(Choice),
                                            std::get<2>(Choice));
        }
      } else {
        assert(false && "multi-choice options must be of enum type");
      }
    }

    _Opts[Option._Name] = Opt;
  }

  bool optPresent(llvm::StringRef Name) const
  { return _OptsPresent.find(Name) != _OptsPresent.end(); }

  template<typename T>
  auto findOpt(llvm::StringRef Name) const
  {
    auto OptIt(_Opts.find(Name));

    assert(OptIt != _Opts.end() && "valid option name");
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

  llvm::cl::OptionCategory _Category;

  std::string _Usage;
  std::vector<llvm::cl::extrahelp> _Help;

  std::unordered_map<std::string, std::any> _Opts;
  std::unordered_set<std::string> _OptsPresent;
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
