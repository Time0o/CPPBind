#ifndef GUARD_TEMPLATE_ARGUMENT_H
#define GUARD_TEMPLATE_ARGUMENT_H

#include <deque>
#include <stdexcept>
#include <string>

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"

#include "Identifier.hpp"
#include "String.hpp"

namespace cppbind
{

class NotATemplateSpecialization : public std::invalid_argument
{
public:
  NotATemplateSpecialization()
  : std::invalid_argument("not a template specialization")
  {}
};

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

  std::string str(bool AsPostfix = false) const;

private:
  clang::TemplateArgument Arg_;
};

class TemplateArgumentList
{
public:
  TemplateArgumentList(clang::FunctionDecl const *Decl)
  : Args_(determineArgs(determineArgList(Decl)))
  {}

  TemplateArgumentList(clang::CXXRecordDecl const *Decl)
  : Args_(determineArgs(determineArgList(Decl)))
  {}

  static bool contains(std::string const &What)
  { return What.find('<') != std::string::npos; }

  static std::string extract(std::string const &What)
  {
    auto Beg(What.find('<'));

    if (Beg == std::string::npos)
      return "";

    auto End(What.rfind('>'));

    return What.substr(Beg + 1, End - 1 - Beg);
  }

  static std::string strip(std::string const &What)
  { return What.substr(0, What.find('<')); }

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

  std::string str(bool AsPostfix = false) const;

private:
  static std::pair<clang::TemplateArgumentList const *, int> determineArgList(
    clang::FunctionDecl const *Decl);

  static std::pair<clang::TemplateArgumentList const *, int> determineArgList(
    clang::CXXRecordDecl const *Decl);

  static std::deque<TemplateArgument> determineArgs(
    std::pair<clang::TemplateArgumentList const *, int> const &ArgList);

  std::deque<TemplateArgument> Args_;
};

} // namespace cppbind

#endif // GUARD_TEMPLATE_ARGUMENT_H
