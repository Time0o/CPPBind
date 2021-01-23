#ifndef GUARD_WRAPPER_RECORD_H
#define GUARD_WRAPPER_RECORD_H

#include <string>

#include "clang/AST/DeclCXX.h"

#include "Identifier.hpp"
#include "WrapperFunction.hpp"
#include "WrapperType.hpp"

namespace cppbind
{

class WrapperRecord
{
public:
  explicit WrapperRecord(clang::CXXRecordDecl const *Decl)
  : Decl_(Decl),
    Type_(Decl_->getTypeForDecl())
  {}

  bool needsImplicitDefaultConstructor() const
  { return Decl_->needsImplicitDefaultConstructor(); }

  WrapperFunction implicitDefaultConstructor() const
  {
    return WrapperFunctionBuilder(qualifiedMemberName("new"), Type_)
           .setReturnType(Type_.pointerTo())
           .isConstructor()
           .build();
  }

  bool needsImplicitDestructor() const
  { return Decl_->needsImplicitDestructor(); }

  WrapperFunction implicitDestructor() const
  {
    return WrapperFunctionBuilder(qualifiedMemberName("delete"), Type_)
           .addParam(Type_.pointerTo(), Identifier("self"))
           .isDestructor()
           .build();
  }

  Identifier name() const
  { return Identifier(Type_.str()); }

private:
  Identifier qualifiedMemberName(std::string const &Name) const
  { return Identifier(Name).qualified(Identifier(Decl_)); }

  clang::CXXRecordDecl const *Decl_;
  WrapperType Type_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_RECORD_H
