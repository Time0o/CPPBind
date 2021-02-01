#ifndef GUARD_WRAPPER_RECORD_H
#define GUARD_WRAPPER_RECORD_H

#include <vector>

#include "clang/AST/DeclCXX.h"

#include "Identifier.hpp"
#include "WrapperFunction.hpp"
#include "WrapperType.hpp"
#include "WrapperVariable.hpp"

namespace cppbind
{

class WrapperRecord
{
public:
  explicit WrapperRecord(WrapperType const &Type);

  explicit WrapperRecord(clang::CXXRecordDecl const *Decl);

  Identifier getName() const
  { return Name_; }

  void setName(Identifier const &Name)
  { Name_ = Name; }

  WrapperType getType() const
  { return Type_; }

  void setType(WrapperType const &Type)
  { Type_ = Type; }

  std::vector<WrapperFunction> getConstructors() const;

  WrapperFunction getDestructor() const;

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
  Identifier Name_;

public:
  std::vector<WrapperVariable> Variables;
  std::vector<WrapperFunction> Functions;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_RECORD_H
