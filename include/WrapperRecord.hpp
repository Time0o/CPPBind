#ifndef GUARD_WRAPPER_RECORD_H
#define GUARD_WRAPPER_RECORD_H

#include <cassert>
#include <memory>
#include <string>
#include <utility>

#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"

#include "Identifier.hpp"
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
    return WrapperFunctionBuilder(qualifiedMemberName("new"), Type_)
           .setReturnType(TypePointer_)
           .isConstructor()
           .build();
  }

  bool needsImplicitDestructor() const
  { return Decl_->needsImplicitDestructor(); }

  WrapperFunction implicitDestructor() const
  {
    return WrapperFunctionBuilder(qualifiedMemberName("delete"), Type_)
           .addParam(TypePointer_, Identifier("self"))
           .isDestructor()
           .build();
  }

  Identifier name() const
  { return Identifier(Type_.str()); }

private:
  Identifier qualifiedMemberName(std::string const &Name) const
  { return Identifier(Name).qualify(Identifier(Decl_)); }

  clang::CXXRecordDecl const *Decl_;
  WrapperType Type_, TypePointer_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_RECORD_H
