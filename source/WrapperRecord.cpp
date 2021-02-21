#include <cassert>
#include <deque>
#include <iterator>
#include <queue>
#include <utility>
#include <vector>

#include "clang/AST/DeclCXX.h"

#include "ClangUtils.hpp"
#include "Identifier.hpp"
#include "WrapperFunction.hpp"
#include "WrapperRecord.hpp"
#include "WrapperType.hpp"
#include "WrapperVariable.hpp"

namespace cppbind
{

WrapperRecord::WrapperRecord(clang::CXXRecordDecl const *Decl)
: Type_(Decl->getTypeForDecl()),
  BaseTypes_(determinePublicBaseTypes(Decl)),
  IsAbstract_(determineIfAbstract(Decl)),
  IsCopyable_(determineIfCopyable(Decl)),
  IsMoveable_(determineIfMoveable(Decl))
{
  Variables = determinePublicMemberVariables(Decl);
  Functions = determinePublicMemberFunctions(Decl);
}

Identifier
WrapperRecord::getName() const
{ return Identifier(Type_.format(false, true)); }

std::vector<WrapperFunction>
WrapperRecord::getConstructors() const
{
  std::vector<WrapperFunction> Constructors;
  for (auto const &Wf : Functions) {
    if (Wf.isConstructor())
      Constructors.push_back(Wf);
  }

  return Constructors;
}

WrapperFunction
WrapperRecord::getDestructor() const
{
  for (auto const &Wf : Functions) {
    if (Wf.isDestructor())
      return Wf;
  }

  assert(false); // XXX
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

std::vector<WrapperVariable>
WrapperRecord::determinePublicMemberVariables(
  clang::CXXRecordDecl const *Decl) const
{ return {}; } // TODO

std::vector<WrapperFunction>
WrapperRecord::determinePublicMemberFunctions(
  clang::CXXRecordDecl const *Decl) const
{
  // determine public and publicy inherited member functions
  std::deque<clang::CXXMethodDecl const *> PublicMethodDecls;

  for (auto const *MethodDecl : Decl->methods()) {
    if (MethodDecl->getAccess() != clang::AS_public)
      continue;

    PublicMethodDecls.push_back(MethodDecl);
  }

  std::queue<clang::CXXBaseSpecifier> BaseQueue;

  for (auto const &Base : Decl->bases())
    BaseQueue.push(Base);

  while (!BaseQueue.empty()) {
    auto Base(BaseQueue.front());
    BaseQueue.pop();

    if (Base.getAccessSpecifier() != clang::AS_public)
      continue;

    for (auto const *MethodDecl : decl::baseDecl(Base)->methods()) {
      if (decl::isConstructor(MethodDecl) || decl::isDestructor(MethodDecl))
        continue;

      if (MethodDecl->getAccess() != clang::AS_public)
        continue;

      PublicMethodDecls.push_back(MethodDecl);
    }

    for (auto const &BaseOfBase : decl::baseDecl(Base)->bases())
      BaseQueue.push(BaseOfBase);
  }

  // prune uninteresting member functions
  std::vector<WrapperFunction> PublicMethods;
  PublicMethods.reserve(PublicMethodDecls.size());

  for (auto const *MethodDecl : PublicMethodDecls) {
    if (MethodDecl->isDeleted())
      continue;

    if (decl::isConstructor(MethodDecl)) {
      if (Decl->isAbstract())
        continue;

      if (decl::asConstructor(MethodDecl)->isCopyConstructor() ||
          decl::asConstructor(MethodDecl)->isMoveConstructor()) {
        continue;
      }
    }

    if (decl::isDestructor(MethodDecl)) {
      if (Decl->isAbstract())
        continue;
    } else if (MethodDecl->size_overridden_methods() != 0) {
      continue;
    }

    if (MethodDecl->isOverloadedOperator())
      continue; // XXX

    PublicMethods.emplace_back(WrapperFunctionBuilder(MethodDecl)
                              .setParent(this)
                              .build());
  }

  // add implicit constructor
  if (Decl->needsImplicitDefaultConstructor() && !Decl->isAbstract())
    PublicMethods.push_back(implicitDefaultConstructor(Decl));

  // add implicit constructor
  if (Decl->needsImplicitDestructor())
    PublicMethods.push_back(implicitDestructor(Decl));

  return PublicMethods;
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
