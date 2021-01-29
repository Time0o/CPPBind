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
  friend class Wrapper;

public:
  explicit WrapperRecord(WrapperType const &Type)
  : Decl_(nullptr),
    Type_(Type)
  {}

  explicit WrapperRecord(clang::CXXRecordDecl const *Decl)
  : Decl_(Decl),
    Type_(Decl_->getTypeForDecl())
  {}

  WrapperFunction implicitDestructor() const
  {
    return WrapperFunctionBuilder(qualifiedMemberName(Identifier::DELETE))
           .setParentType(Type_)
           .setIsDestructor()
           .build();
  }

  WrapperType getType() const
  { return Type_; }

  void setType(WrapperType const &Type)
  { Type_ = Type; }

private:
  bool needsImplicitDefaultConstructor() const
  { return Decl_ && Decl_->needsImplicitDefaultConstructor(); }

  WrapperFunction implicitDefaultConstructor() const
  {
    return WrapperFunctionBuilder(qualifiedMemberName(Identifier::NEW))
           .setParentType(Type_)
           .setIsConstructor()
           .build();
  }

  bool needsImplicitDestructor() const
  { return Decl_ && Decl_->needsImplicitDestructor(); }

  Identifier qualifiedMemberName(std::string const &Name) const
  { return Identifier(Name).qualified(Identifier(Decl_)); }

  clang::CXXRecordDecl const *Decl_;
  WrapperType Type_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_RECORD_H
