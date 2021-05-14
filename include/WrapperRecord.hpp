#ifndef GUARD_WRAPPER_RECORD_H
#define GUARD_WRAPPER_RECORD_H

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "clang/AST/DeclCXX.h"

#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "LLVMFormat.hpp"
#include "Mixin.hpp"
#include "TemplateArgument.hpp"
#include "WrapperFunction.hpp"
#include "WrapperObject.hpp"
#include "WrapperType.hpp"

namespace cppbind
{

class WrapperRecord : public WrapperObject<clang::CXXRecordDecl>,
                      public mixin::NotCopyOrMoveable
{
public:
  WrapperRecord(Identifier const &Name, WrapperType const &Type);

  explicit WrapperRecord(clang::CXXRecordDecl const *Decl);

  static WrapperRecord declaration(clang::CXXRecordDecl const *Decl);

  bool operator==(WrapperRecord const &Other) const
  { return getType() != Other.getType(); }

  bool operator!=(WrapperRecord const &Other) const
  { return !(this->operator==(Other)); }

  void overload(std::shared_ptr<IdentifierIndex> II);

  Identifier getName() const;

  Identifier getFormat(bool WithTemplatePostfix = false) const;

  WrapperType getType() const
  { return Type_; }

  std::deque<WrapperFunction> const &getFunctions() const
  { return Functions_; }

  std::deque<WrapperFunction> &getFunctions()
  { return Functions_; }

  std::deque<WrapperFunction const *> getConstructors() const;
  std::optional<WrapperFunction const *> getDefaultConstructor() const;
  std::optional<WrapperFunction const *> getCopyConstructor() const;
  std::optional<WrapperFunction const *> getCopyAssignmentOperator() const;
  std::optional<WrapperFunction const *> getMoveConstructor() const;
  std::optional<WrapperFunction const *> getMoveAssignmentOperator() const;
  std::optional<WrapperFunction const *> getDestructor() const;

  std::optional<std::string> getTemplateArgumentList() const;

  bool isDefinition() const
  { return IsDefinition_; }

  bool isComplete() const
  { return IsDefinition_; }

  bool isAbstract() const
  { return IsAbstract_; }

  bool isCopyable() const
  { return IsCopyable_; }

  bool isMoveable() const
  { return IsMoveable_; }

  bool isTemplateInstantiation() const
  { return static_cast<bool>(TemplateArgumentList_); }

private:
  std::deque<WrapperFunction> determinePublicMemberFunctions(
    clang::CXXRecordDecl const *Decl) const;

  std::deque<clang::CXXMethodDecl const *>
  determinePublicMemberFunctionDecls(
    clang::CXXRecordDecl const *Decl, bool IncludeInherited = false) const;

  std::deque<clang::CXXMethodDecl const *>
  determineInheritedPublicMemberFunctionDecls(
    clang::CXXRecordDecl const *Decl) const;

  std::deque<clang::CXXMethodDecl const *>
  prunePublicMemberFunctionDecls(
    clang::CXXRecordDecl const *Decl,
    std::deque<clang::CXXMethodDecl const *> const &PublicMethodDecls) const;

  std::deque<std::pair<clang::FieldDecl const *, clang::CXXMethodDecl const *>>
  determinePublicCallableMemberFieldDecls(
    clang::CXXRecordDecl const *Decl) const;

  static bool determineIfDefinition(clang::CXXRecordDecl const *Decl);
  static bool determineIfAbstract(clang::CXXRecordDecl const *Decl);
  static bool determineIfCopyable(clang::CXXRecordDecl const *Decl);
  static bool determineIfMoveable(clang::CXXRecordDecl const *Decl);

  static std::optional<TemplateArgumentList>
  determineTemplateArgumentList(clang::CXXRecordDecl const *Decl);

  WrapperFunction implicitDefaultConstructor(
    clang::CXXRecordDecl const *Decl) const;

  WrapperFunction implicitDestructor(
    clang::CXXRecordDecl const *Decl) const;

  Identifier Name_;
  WrapperType Type_;

  std::deque<WrapperFunction> Functions_;

  bool IsDefinition_ = false;
  bool IsAbstract_ = false;
  bool IsCopyable_ = false;
  bool IsMoveable_ = false;

  std::optional<TemplateArgumentList> TemplateArgumentList_;
};

} // namespace cppbind

namespace llvm { LLVM_FORMAT_PROVIDER(cppbind::WrapperRecord); }

#endif // GUARD_WRAPPER_RECORD_H
