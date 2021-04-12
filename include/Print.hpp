#ifndef GUARD_PRINT_H
#define GUARD_PRINT_H

#include <string>

#include "clang/AST/Stmt.h"
#include "clang/AST/Type.h"
#include "clang/Basic/SourceLocation.h"

#include "CompilerState.hpp"

namespace cppbind
{

namespace print
{

enum Policy
{
  NO_POLICY,
  DEFAULT_POLICY,
  QUALIFIED_POLICY
};

std::string sourceLocation(clang::SourceLocation const &SL);

std::string sourceContent(clang::SourceLocation const &SLBegin,
                          clang::SourceLocation const &SLEnd);

std::string sourceContent(clang::SourceRange const &SR);

std::string stmt(clang::Stmt const *Stmt, Policy P = DEFAULT_POLICY);

std::string qualType(clang::QualType Type, Policy P = DEFAULT_POLICY);

std::string mangledQualType(clang::QualType const &Type);

std::string type(clang::Type const *Type, Policy P = DEFAULT_POLICY);

std::string mangledType(clang::Type const *Type);

} // namespace print

} // namespace cppbind

#endif // GUARD_PRINT_H
