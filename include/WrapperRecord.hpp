#ifndef GUARD_WRAPPER_RECORD_H
#define GUARD_WRAPPER_RECORD_H

#include <cassert>
#include <memory>
#include <string>
#include <utility>

#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"

#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "WrapperType.hpp"

namespace cppbind
{

class WrapperRecord
{
public:
  explicit WrapperRecord(clang::CXXRecordDecl const *Decl)
  : _Decl(Decl),
    _Type(_Decl->getTypeForDecl()),
    _TypePointer(_Type.pointerTo())
  {}

  bool needsImplicitDefaultConstructor() const
  { return _Decl->needsImplicitDefaultConstructor(); }

  WrapperFunction implicitDefaultConstructor() const
  {
    return WrapperFunctionBuilder(qualifiedMemberName(Identifier::New), _Type)
           .setReturnType(_TypePointer)
           .isConstructor()
           .build();
  }

  bool needsImplicitDestructor() const
  { return _Decl->needsImplicitDestructor(); }

  WrapperFunction implicitDestructor() const
  {
    return WrapperFunctionBuilder(qualifiedMemberName(Identifier::Delete), _Type)
           .addParam(_TypePointer, Identifier::Self)
           .isDestructor()
           .build();
  }

  Identifier name() const
  { return _Type.strBaseUnwrapped(); }

  std::string strDeclaration(std::shared_ptr<IdentifierIndex> II) const
  { return _Type.strWrapped(II) + ";"; }

private:
  Identifier qualifiedMemberName(std::string const &Name) const
  { return Identifier(Name).qualify(_Decl); }

  clang::CXXRecordDecl const *_Decl;
  WrapperType _Type, _TypePointer;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_RECORD_H
