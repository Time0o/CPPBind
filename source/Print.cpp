#include <cassert>
#include <string>

#include "clang/AST/ASTContext.h"
#include "clang/AST/Mangle.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/Type.h"
#include "clang/Basic/SourceLocation.h"

#include "llvm/Support/raw_ostream.h"

#include "CompilerState.hpp"
#include "Print.hpp"
#include "String.hpp"

namespace cppbind
{

namespace print
{

clang::PrintingPolicy makePrintingPolicy(Policy P)
{
  clang::PrintingPolicy PP(CompilerState()->getLangOpts());

  switch (P) {
  case NO_POLICY:
    assert(false);
  case DEFAULT_POLICY:
    return PP;
  case QUALIFIED_POLICY:
    PP.FullyQualifiedName = 1u;
    PP.SuppressScope = 0u;
    PP.PrintCanonicalTypes = 1;
    return PP;
  }
}

std::string sourceRange(clang::SourceRange const &SR)
{
  auto const &SM(ASTContext().getSourceManager());

  return string::splitFirst(SR.printToString(SM), ",").first;
}

std::string stmt(clang::Stmt const *Stmt, Policy P)
{
  std::string Str;
  llvm::raw_string_ostream SS(Str);
  Stmt->printPretty(SS, nullptr, makePrintingPolicy(P));
  return Str;
}

std::string qualType(clang::QualType const &Type, Policy P)
{
  switch (P) {
  case NO_POLICY:
    return Type.getAsString();
  default:
    return Type.getAsString(makePrintingPolicy(P));
  }
}

std::string mangledQualType(clang::QualType const &Type)
{
  auto &Ctx(ASTContext());
  auto &Diag(Ctx.getDiagnostics());

  auto *MangleContext = clang::ItaniumMangleContext::create(Ctx, Diag);

  std::string MangledName;
  llvm::raw_string_ostream Os(MangledName);

  MangleContext->mangleTypeName(Type, Os);

  delete MangleContext;

  return MangledName;
}

std::string type(clang::Type const *Type, Policy P)
{ return qualType(clang::QualType(Type, 0), P); }

inline std::string mangledType(clang::Type const *Type)
{ return mangledQualType(clang::QualType(Type, 0)); }

} // namespace print

} // namespace cppbind
