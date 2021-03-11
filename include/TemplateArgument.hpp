#ifndef GUARD_TEMPLATE_ARGUMENT_H
#define GUARD_TEMPLATE_ARGUMENT_H

#include <deque>
#include <sstream>
#include <string>

#include "clang/AST/DeclTemplate.h"

#include "Identifier.hpp"
#include "String.hpp"

namespace cppbind
{

class TemplateArgument
{
public:
  explicit TemplateArgument(clang::TemplateArgument const &Arg)
  : Arg_(Arg)
  {}

  bool operator==(TemplateArgument const &Other) const
  { return Arg_.structurallyEquals(Other.Arg_); }

  bool operator!=(TemplateArgument const &Other) const
  { return !operator==(Other); }

  std::string str() const
  {
    std::string Str;
    llvm::raw_string_ostream SS(Str);

    Arg_.dump(SS);

    return SS.str();
  }

private:
  clang::TemplateArgument Arg_;
};

class TemplateArgumentList
{
public:
  TemplateArgumentList(clang::TemplateArgumentList const &Args)
  {
    for (auto const &Arg : Args.asArray()) {
      if (Arg.getKind() == clang::TemplateArgument::Pack) {
        for (auto const &PackArg : Arg.getPackAsArray())
          Args_.emplace_back(PackArg);
      } else {
        Args_.emplace_back(Arg);
      }
    }
  }

  bool operator==(TemplateArgumentList const &Other) const
  { return Args_ == Other.Args_; }

  bool operator!=(TemplateArgumentList const &Other) const
  { return !operator==(Other); }

  std::deque<TemplateArgument>::size_type size() const
  { return Args_.size(); }

  std::deque<TemplateArgument>::const_iterator begin() const
  { return Args_.begin(); }

  std::deque<TemplateArgument>::const_iterator end() const
  { return Args_.end(); }

  std::string str(bool AsPostfix = false) const
  {
    std::ostringstream SS;

    if (AsPostfix) {
      for (auto const &Arg : Args_)
        SS << "_" << Arg.str();

      auto Str(SS.str());

      string::replaceAll(Str, " ", "_");

      return Identifier(Str).format(Identifier::ORIG_CASE,
                                    Identifier::REPLACE_QUALS);
    } else {
      SS << "<" << Args_.front().str();
      for (std::size_t i = 1; i < Args_.size(); ++i)
        SS << ", " << Args_[i].str();
      SS << ">";

      return SS.str();
    }
  }

private:
  std::deque<TemplateArgument> Args_;
};

} // namespace cppbind

#endif // GUARD_TEMPLATE_ARGUMENT_H
