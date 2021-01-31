#include <vector>

#include "clang/AST/DeclCXX.h"

#include "Identifier.hpp"
#include "WrapperFunction.hpp"
#include "WrapperRecord.hpp"
#include "WrapperVariable.hpp"

namespace cppbind
{

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
      PublicMemberFunctions.emplace_back(MethodDecl);
  }

  return PublicMemberFunctions;
}

WrapperFunction
WrapperRecord::implicitDefaultConstructor(
  clang::CXXRecordDecl const *Decl) const
{
  auto ConstructorName(Identifier(Identifier::NEW).qualified(Identifier(Decl)));

  return WrapperFunctionBuilder(ConstructorName)
         .setParentType(Type_)
         .setIsConstructor()
         .build();
}

WrapperFunction
WrapperRecord::implicitDestructor(
  clang::CXXRecordDecl const *Decl) const
{
  auto DestructorName(Identifier(Identifier::DELETE).qualified(Identifier(Decl)));

  return WrapperFunctionBuilder(DestructorName)
         .setParentType(Type_)
         .setIsDestructor()
         .build();
}

} // namespace cppbind
