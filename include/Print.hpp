#ifndef GUARD_PRINT_H
#define GUARD_PRINT_H

#include <string>

#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/Type.h"

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
      clang::PrintingPolicy const &CurrentPP(
        CompilerState()->getASTContext().getPrintingPolicy());

      return Type.getAsString(CurrentPP);
    }
  }
}


} // namespace cppbind

#endif // GUARD_PRINT_H
