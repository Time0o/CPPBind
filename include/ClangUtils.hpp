#ifndef GUARD_CLANG_UTILS_H
#define GUARD_CLANG_UTILS_H

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/Dynamic/Parser.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"

#include "CompilerState.hpp"
#include "Error.hpp"

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

inline auto const *baseDecl(clang::CXXBaseSpecifier const &Base)
{
  auto const *BaseType(Base.getType()->getAs<clang::RecordType>());

  return llvm::dyn_cast<clang::CXXRecordDecl>(BaseType->getDecl());
}

template<typename T>
bool matchDecl(T const *Decl, llvm::StringRef MatcherSource)
{
  return false;

// XXX fix for clang 8
#if 0
  using namespace clang::ast_matchers;
  using namespace clang::ast_matchers::dynamic;

  Diagnostics Diag;
  auto Matcher = Parser::parseMatcherExpression(MatcherSource, &Diag);

  if (!Matcher)
    throw CPPBindError(Diag.toString());

  struct DeclMatchesCallback : MatchFinder::MatchCallback
  {
    void run(const MatchFinder::MatchResult &) override
    { Result = true; }

    bool Result = false;

  } Callback;

  MatchFinder Finder;
  Finder.addDynamicMatcher(*Matcher, &Callback);

  Finder.match(*Decl, ASTContext());

  return Callback.Result;
#endif
}

} // namespace decl

} // namespace cppbind

#endif // GUARD_CLANG_UTILS_H
