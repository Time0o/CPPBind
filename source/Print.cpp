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

std::string qualType(clang::QualType Type, Policy P)
{
  switch (P) {
  case NO_POLICY:
    return Type.getAsString();
  case DEFAULT_POLICY:
    return Type.getAsString(makePrintingPolicy(P));
  case QUALIFIED_POLICY:
    {
      if (llvm::isa<clang::ElaboratedType>(Type.getTypePtr())) {
        auto ElaboratedType(
          llvm::dyn_cast<clang::ElaboratedType>(Type.getTypePtr()));

        Type = clang::QualType(
          ElaboratedType->desugar().getTypePtr(),
          Type.getQualifiers().getAsOpaqueValue()); // TODO: do we need this? try it out
      }

      return Type.getAsString(makePrintingPolicy(P));
    }
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
