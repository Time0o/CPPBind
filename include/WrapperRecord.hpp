#ifndef GUARD_WRAPPER_RECORD_H
#define GUARD_WRAPPER_RECORD_H

#include <vector>

#include "clang/AST/DeclCXX.h"

#include "WrapperFunction.hpp"
#include "WrapperType.hpp"
#include "WrapperVariable.hpp"

namespace cppbind
{

class WrapperRecord
{
public:
  explicit WrapperRecord(WrapperType const &Type)
  : Type_(Type)
  {}

  explicit WrapperRecord(clang::CXXRecordDecl const *Decl)
  : Type_(Decl->getTypeForDecl()),
    Variables(determinePublicMemberVariables(Decl)),
    Functions(determinePublicMemberFunctions(Decl))
  {}

  WrapperType getType() const
  { return Type_; }

  void setType(WrapperType const &Type)
  { Type_ = Type; }

private:
  std::vector<WrapperVariable> determinePublicMemberVariables(
    clang::CXXRecordDecl const *Decl) const;

  std::vector<WrapperFunction> determinePublicMemberFunctions(
    clang::CXXRecordDecl const *Decl) const;

  WrapperFunction implicitDefaultConstructor(
    clang::CXXRecordDecl const *Decl) const;

  WrapperFunction implicitDestructor(
    clang::CXXRecordDecl const *Decl) const;

  WrapperType Type_;

public:
  std::vector<WrapperVariable> Variables;
  std::vector<WrapperFunction> Functions;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_RECORD_H
