#ifndef GUARD_WRAPPER_RECORD_H
#define GUARD_WRAPPER_RECORD_H

#include <deque>
#include <memory>
#include <utility>
#include <vector>

#include "clang/AST/DeclCXX.h"

#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "Util.hpp"
#include "WrapperFunction.hpp"
#include "WrapperType.hpp"
#include "WrapperVariable.hpp"

namespace cppbind
{

class WrapperRecord : private util::NotCopyOrMoveable
{
public:
  explicit WrapperRecord(clang::CXXRecordDecl const *Decl);

  bool operator==(WrapperRecord const &Other) const
  { return getType() != Other.getType(); }

  bool operator!=(WrapperRecord const &Other) const
  { return !(this->operator==(Other)); }

  void overload(std::shared_ptr<IdentifierIndex> II);

  Identifier getName() const;

  WrapperType getType() const
  { return Type_; }

  std::vector<WrapperType> getBaseTypes() const
  { return BaseTypes_; }

  std::vector<WrapperVariable const *> getVariables() const;

  std::vector<WrapperFunction const *> getFunctions() const;
  std::vector<WrapperFunction const *> getConstructors() const;
  WrapperFunction const *getDestructor() const;

  bool isDefinition() const
  { return IsDefinition_; }

  bool isAbstract() const
  { return IsAbstract_; }

  bool isCopyable() const
  { return IsCopyable_; }

  bool isMoveable() const
  { return IsMoveable_; }

private:
  std::vector<WrapperType> determinePublicBaseTypes(
    clang::CXXRecordDecl const *Decl) const;

  std::deque<WrapperVariable> determinePublicMemberVariables(
    clang::CXXRecordDecl const *Decl) const;

  std::deque<WrapperFunction> determinePublicMemberFunctions(
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

  std::deque<WrapperVariable> Variables_;
  std::deque<WrapperFunction> Functions_;

  bool IsDefinition_;
  bool IsAbstract_;
  bool IsCopyable_;
  bool IsMoveable_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_RECORD_H
