#ifndef GUARD_PRINT_H
#define GUARD_PRINT_H

#include <string>

#include "clang/AST/ASTContext.h"
#include "clang/AST/Mangle.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/Type.h"

#include "llvm/Support/raw_ostream.h"

#include "CompilerState.hpp"

namespace cppbind
{

enum PrintingPolicy
{
  NONE,
  DEFAULT,
  CURRENT
};

inline std::string printQualType(clang::QualType const &Type, PrintingPolicy PP)
{
  switch (PP) {
  case NONE:
    return Type.getAsString();
  case DEFAULT:
    {
      clang::PrintingPolicy DefaultPP(CompilerState()->getLangOpts());

      return Type.getAsString(DefaultPP);
    }
  case CURRENT:
    {
      clang::PrintingPolicy const &CurrentPP(ASTContext().getPrintingPolicy());

      return Type.getAsString(CurrentPP);
    }
  }
}

inline std::string printMangledQualType(clang::QualType const &Type)
{
  auto *MangleContext = clang::ItaniumMangleContext::create(
                          ASTContext(),
                          ASTContext().getDiagnostics());

  std::string MangledName;
  llvm::raw_string_ostream Os(MangledName);

  MangleContext->mangleTypeName(Type, Os);

  delete MangleContext;

  return MangledName;
}

inline std::string printType(clang::Type const *Type, PrintingPolicy PP)
{ return printQualType(clang::QualType(Type, 0), PP); }

inline std::string printMangledType(clang::Type const *Type)
{ return printMangledQualType(clang::QualType(Type, 0)); }

} // namespace cppbind

#endif // GUARD_PRINT_H
