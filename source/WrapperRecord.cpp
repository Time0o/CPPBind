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
  TemplateArgumentList_(determineTemplateArgumentList(Decl)),
  Name_(Decl),
  Type_(Decl->getTypeForDecl()),
  BaseTypes_(determinePublicBaseTypes(Decl)),
  Functions_(determinePublicMemberFunctions(Decl)),
  IsDefinition_(Decl->isThisDeclarationADefinition()),
  IsAbstract_(determineIfAbstract(Decl)),
  IsCopyable_(determineIfCopyable(Decl)),
  IsMoveable_(determineIfMoveable(Decl))
{}

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

std::vector<WrapperFunction const *>
WrapperRecord::getFunctions() const
{
  std::vector<WrapperFunction const *> Functions;

  for (auto const &F : Functions_)
    Functions.push_back(&F);

  return Functions;
}

std::vector<WrapperFunction const *>
WrapperRecord::getConstructors() const
{
  std::vector<WrapperFunction const *> Constructors;

  for (auto const &F : Functions_) {
    if (F.isConstructor())
      Constructors.push_back(&F);
  }

  return Constructors;
}

WrapperFunction const *
WrapperRecord::getDestructor() const
{
  for (auto const &F : Functions_) {
    if (F.isDestructor())
      return &F;
  }

  assert(false); // XXX
}

std::optional<std::string>
WrapperRecord::getTemplateArgumentList() const
{
  if (!isTemplateInstantiation())
    return std::nullopt;

  return TemplateArgumentList_->str();
}

std::vector<WrapperType>
WrapperRecord::determinePublicBaseTypes(
  clang::CXXRecordDecl const *Decl) const
{
  std::vector<WrapperType> BaseTypes;

  for (auto const &Base : Decl->bases()) {
    if (Base.getAccessSpecifier() == clang::AS_public)
      BaseTypes.emplace_back(Base.getType());
  }

  return BaseTypes;
}

std::deque<WrapperFunction>
WrapperRecord::determinePublicMemberFunctions(
  clang::CXXRecordDecl const *Decl) const
{
  auto PublicMethodDecls(determinePublicMemberFunctionDecls(Decl));

  auto InheritedPublicMethodDecls(determineInheritedPublicMemberFunctionDecls(Decl));

  PublicMethodDecls.insert(PublicMethodDecls.end(),
                           InheritedPublicMethodDecls.begin(),
                           InheritedPublicMethodDecls.end());

  PublicMethodDecls = prunePublicMemberFunctionDecls(Decl, PublicMethodDecls);

  std::deque<WrapperFunction> PublicMethods;

  for (auto const *MethodDecl : PublicMethodDecls) {
    PublicMethods.emplace_back(WrapperFunctionBuilder(MethodDecl)
                              .setParent(this)
                              .build());
  }

  if (Decl->needsImplicitDefaultConstructor() && !Decl->isAbstract())
    PublicMethods.push_back(implicitDefaultConstructor(Decl));

  if (Decl->needsImplicitDestructor())
    PublicMethods.push_back(implicitDestructor(Decl));

  return PublicMethods;
}

std::deque<clang::CXXMethodDecl const *>
WrapperRecord::determinePublicMemberFunctionDecls(
  clang::CXXRecordDecl const *Decl) const
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

      auto const *ConstructorDecl =
        llvm::dyn_cast<clang::CXXConstructorDecl>(MethodDecl);

      if (ConstructorDecl->isCopyConstructor() ||
          ConstructorDecl->isMoveConstructor())
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

bool
WrapperRecord::determineIfAbstract(clang::CXXRecordDecl const *Decl)
{ return Decl->isAbstract(); }

bool
WrapperRecord::determineIfCopyable(clang::CXXRecordDecl const *Decl)
{
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
