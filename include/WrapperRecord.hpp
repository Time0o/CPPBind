#ifndef GUARD_WRAPPER_RECORD_H
#define GUARD_WRAPPER_RECORD_H

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "clang/AST/DeclCXX.h"

#include "ClangUtil.hpp"
#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "Util.hpp"
#include "WrapperFunction.hpp"
#include "WrapperObject.hpp"
#include "WrapperType.hpp"
#include "WrapperVariable.hpp"

namespace cppbind
{

class WrapperRecord : public WrapperObject<clang::CXXRecordDecl>,
                      private util::NotCopyOrMoveable
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

  std::vector<WrapperType> getBaseTypes() const
  { return BaseTypes_; }

  std::vector<WrapperVariable const *> getVariables() const;

  std::vector<WrapperFunction const *> getFunctions() const;
  std::vector<WrapperFunction const *> getConstructors() const;
  WrapperFunction const *getDestructor() const;

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
  std::vector<WrapperType> determinePublicBaseTypes(
    clang::CXXRecordDecl const *Decl) const;

  std::deque<WrapperVariable> determinePublicMemberVariables(
    clang::CXXRecordDecl const *Decl) const;

  std::deque<WrapperFunction> determinePublicMemberFunctions(
    clang::CXXRecordDecl const *Decl) const;

  std::deque<clang::CXXMethodDecl const *>
  determinePublicMemberFunctionDecls(
    clang::CXXRecordDecl const *Decl) const;

  std::deque<clang::CXXMethodDecl const *>
  determineInheritedPublicMemberFunctionDecls(
    clang::CXXRecordDecl const *Decl) const;

  std::deque<clang::CXXMethodDecl const *>
  prunePublicMemberFunctionDecls(
    clang::CXXRecordDecl const *Decl,
    std::deque<clang::CXXMethodDecl const *> const &PublicMethodDecls) const;

  static bool determineIfAbstract(clang::CXXRecordDecl const *Decl);
  static bool determineIfCopyable(clang::CXXRecordDecl const *Decl);
  static bool determineIfMoveable(clang::CXXRecordDecl const *Decl);

  static std::optional<clang_util::TemplateArgumentList>
  determineTemplateArgumentList(clang::CXXRecordDecl const *Decl);

  WrapperFunction implicitDefaultConstructor(
    clang::CXXRecordDecl const *Decl) const;

  WrapperFunction implicitDestructor(
    clang::CXXRecordDecl const *Decl) const;

  Identifier Name_;
  WrapperType Type_;
  std::vector<WrapperType> BaseTypes_;

  std::deque<WrapperVariable> Variables_;
  std::deque<WrapperFunction> Functions_;

  bool IsDefinition_;
  bool IsAbstract_;
  bool IsCopyable_;
  bool IsMoveable_;

  std::optional<clang_util::TemplateArgumentList> TemplateArgumentList_;
};

} // namespace cppbind

namespace llvm
{

LLVM_WRAPPER_OBJECT_FORMAT_PROVIDER(cppbind::WrapperRecord, clang::CXXRecordDecl)

} // namespace llvm

#endif // GUARD_WRAPPER_RECORD_H
