#ifndef GUARD_CLANG_UTILS_H
#define GUARD_CLANG_UTILS_H

#include <string>

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/Dynamic/Parser.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"

#include "CompilerState.hpp"
#include "Error.hpp"
#include "String.hpp"

namespace cppbind
{

namespace decl
{

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
inline auto matcher(std::string const &ID, std::string const &MatcherSource)
{
  using namespace clang::ast_matchers;
  using namespace clang::ast_matchers::dynamic;

  llvm::StringRef IDStringRef(ID);
  llvm::StringRef MatcherSourceStringRef(MatcherSource);

  Diagnostics Diag;
  auto Matcher(Parser::parseMatcherExpression(MatcherSourceStringRef, &Diag));

  if (!Matcher)
    throw CPPBindError(Diag.toString());

  if (!Matcher->canConvertTo<T>()) {
    throw CPPBindError(
      string::Builder() << "no valid conversion for '" << ID << "' matcher");
  }

  auto MatcherBound(Matcher->tryBind(IDStringRef));

  if (!MatcherBound) {
    throw CPPBindError(
      string::Builder() << "failed to bind '" << ID << "' matcher");
  }

  return MatcherBound->convertTo<T>();
}

} // namespace decl

} // namespace cppbind

#endif // GUARD_CLANG_UTILS_H
