#ifndef GUARD_WRAPPER_RECORD_H
#define GUARD_WRAPPER_RECORD_H

#include <utility>
#include <vector>

#include "clang/AST/DeclCXX.h"

#include "Identifier.hpp"
#include "Mixin.hpp"
#include "WrapperFunction.hpp"
#include "WrapperType.hpp"
#include "WrapperVariable.hpp"

namespace cppbind
{

class WrapperRecord : private mixin::NotCopyOrMoveable
{
public:
  explicit WrapperRecord(clang::CXXRecordDecl const *Decl);

  Identifier getName() const;

  WrapperType getType() const
  { return Type_; }

  std::vector<WrapperType> getBaseTypes() const
  { return BaseTypes_; }

  std::vector<WrapperFunction> getConstructors() const;
  WrapperFunction getDestructor() const;

  bool isAbstract() const
  { return IsAbstract_; }

  bool isCopyable() const
  { return IsCopyable_; }

  bool isMoveable() const
  { return IsMoveable_; }

private:
  std::vector<WrapperType> determinePublicBaseTypes(
    clang::CXXRecordDecl const *Decl) const;

  std::vector<WrapperVariable> determinePublicMemberVariables(
    clang::CXXRecordDecl const *Decl) const;

  std::vector<WrapperFunction> determinePublicMemberFunctions(
    clang::CXXRecordDecl const *Decl) const;

  static bool determineIfAbstract(clang::CXXRecordDecl const *Decl);
  static bool determineIfCopyable(clang::CXXRecordDecl const *Decl);
  static bool determineIfMoveable(clang::CXXRecordDecl const *Decl);

  WrapperFunction implicitDefaultConstructor(
    clang::CXXRecordDecl const *Decl) const;

  WrapperFunction implicitDestructor(
    clang::CXXRecordDecl const *Decl) const;

  WrapperType Type_;
  std::vector<WrapperType> BaseTypes_;

public:
  std::vector<WrapperVariable> Variables;
  std::vector<WrapperFunction> Functions;

private:
  bool IsAbstract_;
  bool IsCopyable_;
  bool IsMoveable_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_RECORD_H
