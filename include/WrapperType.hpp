#ifndef GUARD_WRAPPER_TYPE_H
#define GUARD_WRAPPER_TYPE_H

#include <cassert>
#include <memory>
#include <string>
#include <utility>

#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"

#include "FundamentalTypes.hpp"
#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "String.hpp"

namespace cppbind
{

class WrapperType
{
public:
  WrapperType(clang::QualType const &Type)
  : _Type(Type)
  { assert(isFundamental() || isWrappable()); }

  WrapperType(clang::Type const *Type)
  : WrapperType(clang::QualType(Type, 0))
  {}

  WrapperType()
  : WrapperType(FundamentalTypes().get("void"))
  {}

  WrapperType(clang::TypeDecl const *Decl)
  : WrapperType(Decl->getTypeForDecl())
  {}

  clang::QualType const &operator*() const
  { return _Type; }

  clang::QualType const *operator->() const
  { return &_Type; }

  WrapperType unqualified() const
  { return WrapperType(_Type.getTypePtr()); }

  bool isQualified() const
  { return _Type.hasQualifiers(); }

  bool isVoid() const
  { return FundamentalTypes().is(_Type.getTypePtr(), "void"); }

  bool isPointer() const
  { return _Type->isPointerType(); }

  bool isClass() const
  { return (*pointee(true))->isClassType(); }

  bool isStruct() const
  { return (*pointee(true))->isStructureType(); }

  WrapperType pointerTo() const
  { return WrapperType(CompilerState()->getASTContext().getPointerType(_Type)); }

  WrapperType pointee(bool recursive = false) const
  {
    if (!recursive)
      return _Type->getPointeeType();

    WrapperType Pointee(*this);
    while (Pointee.isPointer())
      Pointee = Pointee.pointee();

    return Pointee;
  }

  Identifier name() const
  { return strBaseUnwrapped(); }

  std::string strWrapped(std::shared_ptr<IdentifierIndex> II) const
  {
    if (isFundamental() || pointee(true).isFundamental())
      return strUnwrapped();

    auto Wrapped(strUnwrapped());

    if (isClass())
      replaceStr(Wrapped, "class", "struct");

    replaceStr(Wrapped, strBaseUnwrapped(), strBaseWrapped(II));

    return Wrapped;
  }

  std::string strUnwrapped(bool compact = false) const
  {
    auto Unwrapped(_Type.getAsString());

    if (compact) {
      if (isClass())
        removeFirst(Unwrapped, "class ");
      if (isStruct())
        removeFirst(Unwrapped, "struct ");
    }

    return Unwrapped;
  }

  std::string strBaseWrapped(std::shared_ptr<IdentifierIndex> II) const
  { return II->alias(name()).strQualified(TYPE_CASE, true); }

  std::string strBaseUnwrapped() const
  {
    clang::PrintingPolicy PP(CompilerState()->getLangOpts());
    return pointee(true).unqualified()->getAsString(PP);
  }

private:
  static void removeFirst(std::string &Str,
                          std::string const &Remove)
  {
    auto Pos = Str.find(Remove);

    if (Pos != std::string::npos)
      Str.erase(Pos, Remove.length());
  }

  bool isFundamental() const
  { return _Type->isFundamentalType(); }

  bool isWrappable() const
  { return true; } // XXX

  clang::QualType _Type;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_TYPE_H
