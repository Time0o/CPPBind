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
  : Decl_(Decl),
    Type_(Decl_->getTypeForDecl()),
    TypePointer_(Type_.pointerTo())
  {}

  bool needsImplicitDefaultConstructor() const
  { return Decl_->needsImplicitDefaultConstructor(); }

  WrapperFunction implicitDefaultConstructor() const
  {
    return WrapperFunctionBuilder(qualifiedMemberName(Identifier::New), Type_)
           .setReturnType(TypePointer_)
           .isConstructor()
           .build();
  }

  bool needsImplicitDestructor() const
  { return Decl_->needsImplicitDestructor(); }

  WrapperFunction implicitDestructor() const
  {
    return WrapperFunctionBuilder(qualifiedMemberName(Identifier::Delete), Type_)
           .addParam(TypePointer_, Identifier::Self)
           .isDestructor()
           .build();
  }

  Identifier name() const
  { return Type_.str(); }

private:
  Identifier qualifiedMemberName(std::string const &Name) const
  { return Identifier(Name).qualify(Decl_); }

  clang::CXXRecordDecl const *Decl_;
  WrapperType Type_, TypePointer_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_RECORD_H
