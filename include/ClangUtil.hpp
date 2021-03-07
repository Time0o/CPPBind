#ifndef GUARD_CLANG_UTIL_H
#define GUARD_CLANG_UTIL_H

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/Dynamic/Parser.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Error.h"

#include "Logging.hpp"

namespace cppbind
{

inline auto const *declBase(clang::CXXBaseSpecifier const &Base)
{
  auto const *BaseType(Base.getType()->getAs<clang::RecordType>());

  return llvm::dyn_cast<clang::CXXRecordDecl>(BaseType->getDecl());
}

template<typename T>
auto declMatcher(llvm::StringRef ID, llvm::StringRef MatcherSource)
{
  using namespace clang::ast_matchers;
  using namespace clang::ast_matchers::dynamic;

  Diagnostics Diag;
  auto Matcher(Parser::parseMatcherExpression(MatcherSource, &Diag));

  if (!Matcher)
    throw log::exception(Diag.toString());

  if (!Matcher->canConvertTo<T>())
    throw log::exception("no valid conversion for '{0}' matcher", ID);

  auto MatcherBound(Matcher->tryBind(ID));

  if (!MatcherBound)
    throw log::exception("failed to bind '{0}' matcher", ID);

  return MatcherBound->convertTo<T>();
}

} // namespace cppbind

#endif // GUARD_CLANG_UTIL_H
