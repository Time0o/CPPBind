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
#include "CompilerState.hpp"
#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "Logging.hpp"
#include "TemplateArgument.hpp"
#include "WrapperFunction.hpp"
#include "WrapperObject.hpp"
#include "WrapperRecord.hpp"
#include "WrapperType.hpp"

namespace cppbind
{

WrapperRecord::WrapperRecord(clang::CXXRecordDecl const *Decl,
                             bool IncludeDefinition)
: WrapperObject(Decl),
  Name_(Decl),
  Type_(Decl->getTypeForDecl())
{
  if (IncludeDefinition) {
    Functions_ = determinePublicMemberFunctions(Decl);
    IsDefinition_ = determineIfDefinition(Decl);
    IsAbstract_ = determineIfAbstract(Decl);
    IsCopyable_ = determineIfCopyable(Decl);
    IsMoveable_ = determineIfMoveable(Decl);
    TemplateArgumentList_ = determineTemplateArgumentList(Decl);
  }
}

void
WrapperRecord::overload(std::shared_ptr<IdentifierIndex> II)
{
  for (auto &F : Functions_)
    F.overload(II);
}

Identifier
WrapperRecord::getName() const
{ return getFormat(); }

Identifier
WrapperRecord::getFormat(bool WithTemplatePostfix) const
{ return Identifier(getType().format(WithTemplatePostfix)); }

std::vector<WrapperRecord const *>
WrapperRecord::getBases(bool Recursive) const
{ return CompilerState().types()->getRecordBases(this, Recursive); }

std::deque<WrapperFunction const *>
WrapperRecord::getConstructors() const
{
  std::deque<WrapperFunction const *> Constructors;
  for (auto const &F : Functions_) {
    if (F.isConstructor())
      Constructors.push_back(&F);
  }

  return Constructors;
}

std::optional<WrapperFunction const *>
WrapperRecord::getDefaultConstructor() const
{
  for (auto const &F : Functions_) {
    if (F.isConstructor() && F.getParameters().empty())
      return &F;
  }

  return std::nullopt;
}

std::optional<WrapperFunction const *>
WrapperRecord::getCopyConstructor() const
{
  for (auto const &F : Functions_) {
    if (F.isCopyConstructor())
      return &F;
  }

  return std::nullopt;
}

std::optional<WrapperFunction const *>
WrapperRecord::getCopyAssignmentOperator() const
{
  for (auto const &F : Functions_) {
    if (F.isCopyAssignmentOperator())
      return &F;
  }

  return std::nullopt;
}

std::optional<WrapperFunction const *>
WrapperRecord::getMoveConstructor() const
{
  for (auto const &F : Functions_) {
    if (F.isMoveConstructor())
      return &F;
  }

  return std::nullopt;
}

std::optional<WrapperFunction const *>
WrapperRecord::getMoveAssignmentOperator() const
{
  for (auto const &F : Functions_) {
    if (F.isMoveAssignmentOperator())
      return &F;
  }

  return std::nullopt;
}

std::optional<WrapperFunction const *>
WrapperRecord::getDestructor() const
{
  for (auto const &F : Functions_) {
    if (F.isDestructor())
      return &F;
  }

  return std::nullopt;
}

std::optional<WrapperFunction const *>
WrapperRecord::getBaseCast(bool Const) const
{
  for (auto const &F : Functions_) {
    if (F.isBaseCast() && *F.getOrigin() == *this && F.isConst() == Const)
      return &F;
  }

  return std::nullopt;
}

std::optional<std::string>
WrapperRecord::getTemplateArgumentList() const
{
  if (!isTemplateInstantiation())
    return std::nullopt;

  return TemplateArgumentList_->str();
}

bool
WrapperRecord::isPolymorphic() const
{
  for (auto const &F : Functions_) {
    if (F.isVirtual())
      return true;
  }

  return false;
}

std::deque<WrapperFunction>
WrapperRecord::determinePublicMemberFunctions(
  clang::CXXRecordDecl const *Decl) const
{
  if (!Decl->isThisDeclarationADefinition())
    return {};

  std::deque<WrapperFunction> PublicMemberFunctions;

  // member functions
  auto PublicMemberFunctionsDecls(determinePublicMemberFunctionDecls(Decl, true));

  PublicMemberFunctionsDecls =
    prunePublicMemberFunctionDecls(Decl, PublicMemberFunctionsDecls);

  for (auto const *MethodDecl : PublicMemberFunctionsDecls)
    PublicMemberFunctions.emplace_back(MethodDecl);

  // callable member fields
  auto PublicCallableFieldDecls(determinePublicCallableMemberFieldDecls(Decl));

  for (auto const &[FieldDecl, MethodDecl] : PublicCallableFieldDecls) {
    PublicMemberFunctions.emplace_back(WrapperFunctionBuilder(MethodDecl)
                                       .setName(Identifier(FieldDecl))
                                       .build());
  }

  // base casts
  bool IncludeSelfCast = false;
  for (auto const &F : PublicMemberFunctions) {
    if (F.isVirtual()) {
      IncludeSelfCast = true;
      break;
    }
  }

  for (auto const &BaseCast : determineBaseCasts(Decl, IncludeSelfCast))
    PublicMemberFunctions.emplace_back(BaseCast);

  // reparent
  for (auto &F : PublicMemberFunctions) {
    F = WrapperFunctionBuilder(F)
        .setParent(this)
        .build();
  }

  return PublicMemberFunctions;
}

std::deque<WrapperFunction>
WrapperRecord::determineBaseCasts(clang::CXXRecordDecl const *Decl,
                                  bool IncludeSelfCast) const
{
  std::deque<WrapperFunction> BaseCasts;

  WrapperType SelfType(Decl->getTypeForDecl());
  std::vector<WrapperType> BaseTypes;

  if (IncludeSelfCast)
    BaseTypes.push_back(SelfType);

  for (auto const &BaseType : SelfType.baseTypes(true))
    BaseTypes.push_back(BaseType);

  for (auto const &BaseType : BaseTypes) {
    if (BaseType != SelfType && !BaseType.isPolymorphic())
      continue;

    for (bool Const : {true, false}) {
      auto SelfPointerType = Const ? SelfType.withConst().pointerTo() : SelfType.pointerTo();
      auto BasePointerType = Const ? BaseType.withConst().pointerTo() : BaseType.pointerTo();

      std::string BaseCastName("cast_to_");

      if (Const)
        BaseCastName += "const_";

      BaseCastName += BaseType.format(true, "", "",
                                      Identifier::SNAKE_CASE,
                                      Identifier::REMOVE_QUALS);

      BaseCasts.push_back(
        WrapperFunctionBuilder(Identifier(BaseCastName))
                               .setOrigin(BaseType)
                               .setCustomAction("static_cast<" + BasePointerType.str() + ">")
                               .setReturnType(BasePointerType)
                               .setIsBaseCast()
                               .setIsConst(Const)
                               .setIsNoexcept()
                               .setIsVirtual()
                               .build());
    }
  }

  return BaseCasts;
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
WrapperRecord::determineIfDefinition(clang::CXXRecordDecl const *Decl)
{ return Decl->isThisDeclarationADefinition(); }

bool
WrapperRecord::determineIfAbstract(clang::CXXRecordDecl const *Decl)
{
  if (!Decl->isThisDeclarationADefinition())
    return false;

 return Decl->isAbstract();
}

bool
WrapperRecord::determineIfCopyable(clang::CXXRecordDecl const *Decl)
{
  if (!Decl->isThisDeclarationADefinition())
    return false;

  unsigned NumCopyConstructors = 0u;
  bool CopyConstructorDeleted = false;

  for (auto ctor : Decl->ctors()) {
    if (ctor->isCopyConstructor()) {
      ++NumCopyConstructors;
      if (ctor->isDeleted())
        CopyConstructorDeleted = true;
    }
  }

  if (NumCopyConstructors > 1u)
    throw log::exception("multiple copy constructors defined for {}", Decl->getNameAsString());

  return NumCopyConstructors == 1 && !CopyConstructorDeleted;
}

bool
WrapperRecord::determineIfMoveable(clang::CXXRecordDecl const *Decl)
{
  if (!Decl->isThisDeclarationADefinition())
    return false;

  unsigned NumMoveConstructors = 0u;
  bool MoveConstructorDeleted = false;

  for (auto ctor : Decl->ctors()) {
    if (ctor->isMoveConstructor()) {
      ++NumMoveConstructors;
      if (ctor->isDeleted())
        MoveConstructorDeleted = true;
    }
  }

  if (NumMoveConstructors > 1u)
    throw log::exception("multiple copy constructors defined for {}", Decl->getNameAsString());

  return NumMoveConstructors == 1 && !MoveConstructorDeleted;
}

std::optional<TemplateArgumentList>
WrapperRecord::determineTemplateArgumentList(clang::CXXRecordDecl const *Decl)
{
  try {
    return TemplateArgumentList(Decl);
  } catch (NotATemplateSpecialization const &) {
    return std::nullopt;
  }
}

} // namespace cppbind
