#include <cassert>
#include <vector>

#include "clang/AST/DeclCXX.h"

#include "Identifier.hpp"
#include "WrapperFunction.hpp"
#include "WrapperRecord.hpp"
#include "WrapperType.hpp"
#include "WrapperVariable.hpp"

namespace cppbind
{

WrapperRecord::WrapperRecord(WrapperType const &Type)
: Type_(Type),
  Name_(Identifier(Type_.format(true)))
{}

WrapperRecord::WrapperRecord(clang::CXXRecordDecl const *Decl)
: WrapperRecord(WrapperType(Decl->getTypeForDecl()))
{
  Variables = determinePublicMemberVariables(Decl);
  Functions = determinePublicMemberFunctions(Decl);
}

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

std::vector<WrapperVariable>
WrapperRecord::determinePublicMemberVariables(
  clang::CXXRecordDecl const *Decl) const
{ return {}; } // TODO

std::vector<WrapperFunction>
WrapperRecord::determinePublicMemberFunctions(
  clang::CXXRecordDecl const *Decl) const
{
  std::vector<WrapperFunction> PublicMemberFunctions;

  if (Decl->needsImplicitDefaultConstructor())
    PublicMemberFunctions.push_back(implicitDefaultConstructor(Decl));

  if (Decl->needsImplicitDestructor())
    PublicMemberFunctions.push_back(implicitDestructor(Decl));

  for (auto const *MethodDecl : Decl->methods()) {
    if (MethodDecl->getAccess() == clang::AS_public && !MethodDecl->isDeleted())
      PublicMemberFunctions.emplace_back(this, MethodDecl);
  }

  return PublicMemberFunctions;
}

WrapperFunction
WrapperRecord::implicitDefaultConstructor(
  clang::CXXRecordDecl const *Decl) const
{
  auto ConstructorName(Identifier(Identifier::NEW).qualified(Identifier(Decl)));

  return WrapperFunctionBuilder(ConstructorName)
         .setParent(this)
         .setIsConstructor()
         .build();
}

WrapperFunction
WrapperRecord::implicitDestructor(
  clang::CXXRecordDecl const *Decl) const
{
  auto DestructorName(Identifier(Identifier::DELETE).qualified(Identifier(Decl)));

  return WrapperFunctionBuilder(DestructorName)
         .setParent(this)
         .setIsDestructor()
         .build();
}

} // namespace cppbind
