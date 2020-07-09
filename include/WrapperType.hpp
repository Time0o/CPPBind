#ifndef GUARD_WRAPPER_TYPE_H
#define GUARD_WRAPPER_TYPE_H

#include <cassert>
#include <memory>
#include <string>
#include <utility>

#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"

#include "BuiltinTypes.hpp"
#include "Identifier.hpp"

namespace cppbind
{

class WrapperType
{
public:
  WrapperType(clang::QualType const &Type)
  : _Type(Type)
  { assert(isBuiltin() || isWrappable()); }

  WrapperType(clang::Type const *Type)
  : WrapperType(clang::QualType(Type, 0))
  {}

  WrapperType()
  : WrapperType(getBuiltinType("void"))
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

  bool isVoid() const
  { return _Type->isSpecificBuiltinType(getBuiltinType("void")->getKind()); }

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

  std::string strWrapped() const
  {
    if (isBuiltin())
      return strUnwrapped();

    auto Wrapped(strUnwrapped());

    if (isClass())
      replaceFirst(Wrapped, "class", "struct");

    replaceFirst(Wrapped, strBaseUnwrapped(), strBaseWrapped());

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

  std::string strBaseWrapped() const
  { return Identifier(strBaseUnwrapped()).strQualified("type-case", true); }

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

  static void replaceFirst(std::string &Str,
                           std::string const &Old,
                           std::string const &New)
  {
    auto Pos = Str.find(Old);

    if (Pos != std::string::npos)
      Str.replace(Pos, Old.length(), New);
  }

  bool isBuiltin() const
  { return _Type->isBuiltinType(); }

  bool isWrappable() const
  { return true; } // XXX

  clang::QualType _Type;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_TYPE_H
