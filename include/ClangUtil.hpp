#ifndef GUARD_CLANG_UTIL_H
#define GUARD_CLANG_UTIL_H

#include <cassert>
#include <deque>
#include <sstream>
#include <stdexcept>
#include <string>

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/Dynamic/Parser.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FormatVariadic.h"

#include "CompilerState.hpp"

namespace clang_util
{

inline bool isMethod(clang::FunctionDecl const *Decl)
{ return llvm::isa<clang::CXXMethodDecl>(Decl); };

inline auto const *asMethod(clang::FunctionDecl const *Decl)
{ return llvm::dyn_cast<clang::CXXMethodDecl>(Decl); }

inline bool isConstructor(clang::FunctionDecl const *Decl)
{ return llvm::isa<clang::CXXConstructorDecl>(Decl); };

inline auto const *asConstructor(clang::FunctionDecl const *Decl)
{ return llvm::dyn_cast<clang::CXXConstructorDecl>(Decl); }

inline bool isDestructor(clang::FunctionDecl const *Decl)
{ return llvm::isa<clang::CXXDestructorDecl>(Decl); }

inline auto const *asDestructor(clang::FunctionDecl const *Decl)
{ return llvm::dyn_cast<clang::CXXDestructorDecl>(Decl); }

inline auto const *base(clang::CXXBaseSpecifier const &Base)
{
  auto const *BaseType(Base.getType()->getAs<clang::RecordType>());

  return llvm::dyn_cast<clang::CXXRecordDecl>(BaseType->getDecl());
}

template<typename T>
auto matcher(llvm::StringRef ID, llvm::StringRef MatcherSource)
{
  using namespace clang::ast_matchers;
  using namespace clang::ast_matchers::dynamic;

  Diagnostics Diag;
  auto Matcher(Parser::parseMatcherExpression(MatcherSource, &Diag));

  if (!Matcher)
    throw std::invalid_argument(Diag.toString());

  if (!Matcher->canConvertTo<T>())
    throw std::invalid_argument(
      llvm::formatv("no valid conversion for '{0}' matcher", ID));

  auto MatcherBound(Matcher->tryBind(ID));

  if (!MatcherBound)
    throw std::runtime_error(llvm::formatv("failed to bind '{0}' matcher", ID));

  return MatcherBound->convertTo<T>();
}

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
    } else {
      SS << "<" << Args_.front().str();
      for (std::size_t i = 1; i < Args_.size(); ++i)
        SS << ", " << Args_[i].str();
      SS << ">";
    }

    return SS.str();
  }

private:
  std::deque<TemplateArgument> Args_;
};

} // namespace clang_util

#endif // GUARD_CLANG_UTIL_H
