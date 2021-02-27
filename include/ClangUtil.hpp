#ifndef GUARD_CLANG_UTIL_H
#define GUARD_CLANG_UTIL_H

#include <stdexcept>

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
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

} // namespace clang_util

#endif // GUARD_CLANG_UTIL_H
