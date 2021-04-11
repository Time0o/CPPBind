#include <cassert>
#include <deque>
#include <iterator>
#include <memory>
#include <optional>
#include <queue>
#include <utility>
#include <vector>

#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"

#include "ClangUtil.hpp"
#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "Logging.hpp"
#include "TemplateArgument.hpp"
#include "WrapperFunction.hpp"
#include "WrapperObject.hpp"
#include "WrapperRecord.hpp"
#include "WrapperType.hpp"
#include "WrapperConstant.hpp"

namespace cppbind
{

WrapperRecord::WrapperRecord(clang::CXXRecordDecl const *Decl)
: WrapperObject(Decl),
  Name_(Decl),
  Type_(Decl->getTypeForDecl()),
  Functions_(determinePublicMemberFunctions(Decl)),
  IsDefinition_(Decl->isThisDeclarationADefinition()),
  IsAbstract_(determineIfAbstract(Decl)),
  IsCopyable_(determineIfCopyable(Decl)),
  IsMoveable_(determineIfMoveable(Decl)),
  TemplateArgumentList_(determineTemplateArgumentList(Decl))
{
  for (auto &F : Functions_) {
    F = WrapperFunctionBuilder(F)
        .setParent(this)
        .build();
  }
}

void
WrapperRecord::overload(std::shared_ptr<IdentifierIndex> II)
{
  for (auto &F : Functions_)
    F.overload(II);
}

Identifier
WrapperRecord::getName(bool WithTemplatePostfix) const
{
  Identifier Name(Name_);

  if (WithTemplatePostfix && isTemplateInstantiation())
    Name = Identifier(Name.str() + TemplateArgumentList_->str(true));

  return Name;
}

std::optional<std::string>
WrapperRecord::getTemplateArgumentList() const
{
  if (!isTemplateInstantiation())
    return std::nullopt;

  return TemplateArgumentList_->str();
}

std::deque<WrapperFunction>
WrapperRecord::determinePublicMemberFunctions(
  clang::CXXRecordDecl const *Decl) const
{
  if (!Decl->isThisDeclarationADefinition())
    return {};

  std::deque<WrapperFunction> PublicMethods;

  // member functions
  auto PublicMethodDecls(determinePublicMemberFunctionDecls(Decl, true));

  PublicMethodDecls = prunePublicMemberFunctionDecls(Decl, PublicMethodDecls);

  for (auto const *MethodDecl : PublicMethodDecls)
    PublicMethods.emplace_back(MethodDecl);

  // callable member fields
  auto PublicCallableFieldDecls(determinePublicCallableMemberFieldDecls(Decl));

  for (auto const &[FieldDecl, MethodDecl] : PublicCallableFieldDecls) {
    PublicMethods.emplace_back(WrapperFunctionBuilder(MethodDecl)
                               .setName(Identifier(FieldDecl))
                               .build());
  }

  return PublicMethods;
}

std::deque<clang::CXXMethodDecl const *>
WrapperRecord::determinePublicMemberFunctionDecls(
  clang::CXXRecordDecl const *Decl, bool IncludeInherited) const
{
  std::deque<clang::CXXMethodDecl const *> PublicMethodDecls;

  for (auto const *MethodDecl : Decl->methods()) {
    if (MethodDecl->getAccess() == clang::AS_public)
      PublicMethodDecls.push_back(MethodDecl);
  }

  for (auto const *InnerDecl :
       static_cast<clang::DeclContext const *>(Decl)->decls()) {

    auto const *FunctionTemplateDecl =
      llvm::dyn_cast<clang::FunctionTemplateDecl>(InnerDecl);

    if (!FunctionTemplateDecl)
      continue;

    for (auto const *FunctionDecl : FunctionTemplateDecl->specializations()) {
      assert(FunctionDecl);

      auto const *MethodDecl = llvm::dyn_cast<clang::CXXMethodDecl>(FunctionDecl);
      assert(MethodDecl);

      if (MethodDecl->getAccess() == clang::AS_public)
        PublicMethodDecls.push_back(MethodDecl);
    }
  }

  if (IncludeInherited) {
    auto InheritedPublicMethodDecls(
      determineInheritedPublicMemberFunctionDecls(Decl));

    PublicMethodDecls.insert(PublicMethodDecls.end(),
                             InheritedPublicMethodDecls.begin(),
                             InheritedPublicMethodDecls.end());
  }

  return PublicMethodDecls;
}

std::deque<clang::CXXMethodDecl const *>
WrapperRecord::determineInheritedPublicMemberFunctionDecls(
  clang::CXXRecordDecl const *Decl) const
{
  std::deque<clang::CXXMethodDecl const *> PublicMethodDecls;

  std::queue<clang::CXXBaseSpecifier> BaseQueue;

  for (auto const &Base : Decl->bases())
    BaseQueue.push(Base);

  while (!BaseQueue.empty()) {
    auto Base(BaseQueue.front());
    BaseQueue.pop();

    if (Base.getAccessSpecifier() != clang::AS_public)
      continue;

    for (auto const *MethodDecl :
         determinePublicMemberFunctionDecls(declBase(Base))) {
      if (!llvm::isa<clang::CXXConstructorDecl>(MethodDecl) &&
          !llvm::isa<clang::CXXDestructorDecl>(MethodDecl) &&
          !MethodDecl->isCopyAssignmentOperator() &&
          !MethodDecl->isMoveAssignmentOperator())
        PublicMethodDecls.push_back(MethodDecl);

    }

    for (auto const &BaseOfBase : declBase(Base)->bases())
      BaseQueue.push(BaseOfBase);
  }

  return PublicMethodDecls;
}

std::deque<clang::CXXMethodDecl const *>
WrapperRecord::prunePublicMemberFunctionDecls(
  clang::CXXRecordDecl const *Decl,
  std::deque<clang::CXXMethodDecl const *> const &PublicMethodDecls) const
{
  std::deque<clang::CXXMethodDecl const *> PrunedPublicMethodDecls;

  for (auto const *MethodDecl : PublicMethodDecls) {
    if (MethodDecl->isDeleted())
      continue;

    if (llvm::isa<clang::CXXConstructorDecl>(MethodDecl)) {
      if (Decl->isAbstract())
        continue;
    }

    if (llvm::isa<clang::CXXDestructorDecl>(MethodDecl)) {
      if (Decl->isAbstract())
        continue;
    } else if (MethodDecl->size_overridden_methods() != 0) {
      continue;
    }

    PrunedPublicMethodDecls.push_back(MethodDecl);
  }

  return PrunedPublicMethodDecls;
}

std::deque<std::pair<clang::FieldDecl const *, clang::CXXMethodDecl const *>>
WrapperRecord::determinePublicCallableMemberFieldDecls(
  clang::CXXRecordDecl const *Decl) const
{
  std::deque<std::pair<clang::FieldDecl const *, clang::CXXMethodDecl const *>>
  PublicCallableMemberFieldDecls;

  for (auto const *FieldDecl : Decl->fields()) {
    if (FieldDecl->getAccess() != clang::AS_public)
      continue;

    auto const *RecordFieldDecl = FieldDecl->getType()->getAsCXXRecordDecl();
    if (!RecordFieldDecl)
      continue;

    for (auto const *MethodDecl :
         determinePublicMemberFunctionDecls(RecordFieldDecl, true))

      if (MethodDecl->isOverloadedOperator() &&
          MethodDecl->getOverloadedOperator() == clang::OO_Call)
        PublicCallableMemberFieldDecls.emplace_back(FieldDecl, MethodDecl);
  }

  return PublicCallableMemberFieldDecls;
}

bool
WrapperRecord::determineIfAbstract(clang::CXXRecordDecl const *Decl)
{ return Decl->isThisDeclarationADefinition() && Decl->isAbstract(); }

bool
WrapperRecord::determineIfCopyable(clang::CXXRecordDecl const *Decl)
{
  if (!Decl->isThisDeclarationADefinition())
    return false;

  // XXX likely not always correct
  // XXX can there be more than one 'copy constructor'?

  for (auto ctor : Decl->ctors()) {
    if (ctor->isCopyConstructor() && ctor->isDeleted())
      return false;
  }

  return true;
}

bool
WrapperRecord::determineIfMoveable(clang::CXXRecordDecl const *Decl)
{
  if (!Decl->isThisDeclarationADefinition())
    return false;

  // XXX likely not always correct
  // XXX can there be more than one 'move constructor'?

  for (auto ctor : Decl->ctors()) {
    if (ctor->isMoveConstructor() && ctor->isDeleted())
      return false;
  }

  return true;
}

std::optional<TemplateArgumentList>
WrapperRecord::determineTemplateArgumentList(clang::CXXRecordDecl const *Decl)
{
  if (!llvm::isa<clang::ClassTemplateSpecializationDecl>(Decl))
    return std::nullopt;

  return TemplateArgumentList(
    llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(Decl)->getTemplateArgs());
}

WrapperFunction
WrapperRecord::implicitDefaultConstructor(
  clang::CXXRecordDecl const *Decl) const
{
  auto ConstructorName(Identifier(Identifier::NEW).qualified(Identifier(Decl)));

  return WrapperFunctionBuilder(ConstructorName)
         .setIsConstructor()
         .setParent(this) // XXX
         .build();
}

WrapperFunction
WrapperRecord::implicitDestructor(
  clang::CXXRecordDecl const *Decl) const
{
  auto DestructorName(Identifier(Identifier::DELETE).qualified(Identifier(Decl)));

  return WrapperFunctionBuilder(DestructorName)
         .setIsDestructor()
         .setIsNoexcept() // XXX unless base class destructor can throw
         .setParent(this) // XXX
         .build();
}

} // namespace cppbind
