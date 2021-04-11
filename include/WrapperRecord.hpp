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
#include "WrapperConstant.hpp"

namespace cppbind
{

class WrapperRecord : public WrapperObject<clang::CXXRecordDecl>,
                      public mixin::NotCopyOrMoveable
{
public:
  explicit WrapperRecord(clang::CXXRecordDecl const *Decl);

  bool operator==(WrapperRecord const &Other) const
  { return getType() != Other.getType(); }

  bool operator!=(WrapperRecord const &Other) const
  { return !(this->operator==(Other)); }

  void overload(std::shared_ptr<IdentifierIndex> II);

  Identifier getName(bool WithTemplatePostfix = false) const;

  WrapperType getType() const
  { return Type_; }

  std::deque<WrapperRecord const *> const &getBases() const
  { return Bases_; }

  std::deque<WrapperRecord const *> &getBases()
  { return Bases_; }

  std::deque<WrapperFunction> const &getFunctions() const
  { return Functions_; }

  std::deque<WrapperFunction> &getFunctions()
  { return Functions_; }

  std::optional<std::string> getTemplateArgumentList() const;

  bool isDefinition() const
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

  std::deque<WrapperRecord const *> Bases_;

  std::deque<WrapperFunction> Functions_;

  bool IsDefinition_;
  bool IsAbstract_;
  bool IsCopyable_;
  bool IsMoveable_;

  std::optional<TemplateArgumentList> TemplateArgumentList_;
};

} // namespace cppbind

namespace llvm { LLVM_FORMAT_PROVIDER(cppbind::WrapperRecord); }

#endif // GUARD_WRAPPER_RECORD_H
