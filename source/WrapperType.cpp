#include <cassert>
#include <optional>
#include <stack>
#include <string>
#include <vector>

#include "llvm/ADT/StringRef.h"

#include "CompilerState.hpp"
#include "FundamentalTypes.hpp"
#include "Identifier.hpp"
#include "Print.hpp"
#include "String.hpp"
#include "TemplateArgument.hpp"
#include "WrapperType.hpp"

namespace cppbind
{

WrapperType::WrapperType()
: WrapperType("void")
{}

WrapperType::WrapperType(clang::QualType const &Type)
: Type_(determineDesugaredType(Type)),
  TemplateArgumentList_(determineTemplateArgumentList(Type))
{}

WrapperType::WrapperType(clang::Type const *Type)
: WrapperType(clang::QualType(Type, 0u))
{}

WrapperType::WrapperType(std::string const &TypeName)
: WrapperType(FundamentalTypes().get(TypeName))
{}

WrapperType::WrapperType(clang::TypeDecl const *Decl)
: WrapperType(Decl->getTypeForDecl())
{}

bool
WrapperType::operator==(WrapperType const &Other) const
{ return Other.mangled() == mangled(); }

bool
WrapperType::operator<(WrapperType const &Other) const
{ return mangled() < Other.mangled(); }

bool
WrapperType::isTemplateInstantiation() const
{ return static_cast<bool>(TemplateArgumentList_); }

bool
WrapperType::isFundamental(char const *Which) const
{
  if (!type()->isFundamentalType())
    return false;

  if (!Which)
    return true;

  return FundamentalTypes().is(typePtr(), Which);
}

bool
WrapperType::isVoid() const
{ return type()->isVoidType(); }

bool
WrapperType::isBoolean() const
{ return isFundamental("bool"); }

bool
WrapperType::isCString() const
{ return operator==(WrapperType("char").withConst().pointerTo()); }

bool
WrapperType::isEnum() const
{ return type()->isEnumeralType(); }

bool
WrapperType::isScopedEnum() const
{ return type()->isScopedEnumeralType(); }

bool
WrapperType::isIntegral() const
{
  if (!type()->isIntegralType(ASTContext()))
    return false;

  return !isBoolean() && !isScopedEnum();
}

bool
WrapperType::isFloating() const
{ return type()->isFloatingType(); }

bool
WrapperType::isReference() const
{ return type()->isReferenceType(); }

bool
WrapperType::isLValueReference() const
{ return type()->isLValueReferenceType(); }

bool
WrapperType::isRValueReference() const
{ return type()->isRValueReferenceType(); }

bool
WrapperType::isPointer() const
{ return type()->isPointerType(); }

bool
WrapperType::isRecord() const
{ return type()->isRecordType(); }

bool
WrapperType::isStruct() const
{ return type()->isStructureType(); }

bool
WrapperType::isClass() const
{ return type()->isClassType(); }

bool
WrapperType::isConst() const
{ return type().isConstQualified(); }

WrapperType
WrapperType::lvalueReferenceTo() const
{ return WrapperType(ASTContext().getLValueReferenceType(type())); }

WrapperType
WrapperType::rvalueReferenceTo() const
{ return WrapperType(ASTContext().getRValueReferenceType(type())); }

WrapperType
WrapperType::referenced() const
{ return WrapperType(type().getNonReferenceType()); }

WrapperType
WrapperType::pointerTo(unsigned Repeat) const
{
  WrapperType PointerTo(ASTContext().getPointerType(type()));

  return Repeat == 0u ? PointerTo : PointerTo.pointerTo(Repeat - 1u);
}

WrapperType
WrapperType::pointee(bool Recursive) const
{
  if (!Recursive)
    return WrapperType(type()->getPointeeType());

  WrapperType Pointee(*this);
  while (Pointee.isPointer())
    Pointee = Pointee.pointee();

  return Pointee;
}

WrapperType
WrapperType::underlyingIntegerType() const
{
  assert(isEnum());

  auto const *EnumType = type()->getAs<clang::EnumType>();

  return WrapperType(EnumType->getDecl()->getIntegerType());
}

unsigned
WrapperType::qualifiers() const
{ return type().getQualifiers().getAsOpaqueValue(); }

WrapperType
WrapperType::qualified(unsigned Qualifiers) const
{ return WrapperType(requalifyType(type(), Qualifiers)); }

WrapperType
WrapperType::unqualified() const
{ return WrapperType(typePtr()); }

WrapperType
WrapperType::withConst() const
{ return WrapperType(type().withConst()); }

WrapperType
WrapperType::withoutConst() const
{
  auto TypeCopy(type());
  TypeCopy.removeLocalConst();

  return WrapperType(TypeCopy);
}

WrapperType
WrapperType::getBase() const
{ return referenced().pointee(true); }

void
WrapperType::setBase(WrapperType const &NewBase)
{
  enum Indirection
  {
    POINTER,
    LVALUE_REFERENCE,
    RVALUE_REFERENCE
  };

  std::stack<Indirection> Indirections;

  WrapperType NewType(*this);

  if (NewType.isLValueReference()) {
    Indirections.push(LVALUE_REFERENCE);
    NewType = NewType.referenced();
  } else if (NewType.isRValueReference()) {
    Indirections.push(RVALUE_REFERENCE);
    NewType = NewType.referenced();
  }

  while (NewType.isPointer()) {
    Indirections.push(POINTER);
    NewType = NewType.pointee();
  }

  NewType = NewBase;

  while (!Indirections.empty()) {
    switch (Indirections.top()) {
    case POINTER:
      NewType = NewType.pointerTo();
      break;
    case LVALUE_REFERENCE:
      NewType = NewType.lvalueReferenceTo();
      break;
    case RVALUE_REFERENCE:
      NewType = NewType.rvalueReferenceTo();
      break;
    }

    Indirections.pop();
  }

  *this = NewType;
}

std::size_t
WrapperType::size() const
{ return ASTContext().getTypeInfo(type()).Width; }

std::string
WrapperType::str(bool WithTemplatePostfix) const
{ return format(WithTemplatePostfix); }

std::string
WrapperType::format(bool WithTemplatePostfix,
                    std::string const &WithPrefix,
                    std::string const &WithPostfix,
                    Identifier::Case Case,
                    Identifier::Quals Quals) const
{
  auto Base(getBase());

  if (Base.isRecord()) {
    auto Str(print::qualType(type()));

    auto StrBase(print::qualType(requalifyType(Base.type(), 0u)));

    auto StrReplace = StrBase;

    if (WithTemplatePostfix && Base.isTemplateInstantiation()) {
      StrReplace = TemplateArgumentList::strip(StrReplace)
                 + Base.TemplateArgumentList_->str(true);
    }

    StrReplace = Identifier(StrReplace).format(Case, Quals);

    StrReplace = WithPrefix + StrReplace + WithPostfix;

    string::replace(Str, StrBase, StrReplace);

    return Str;

  } else {
    return print::qualType(type());
  }
}

std::string
WrapperType::mangled() const
{ return print::mangledQualType(type()); }

clang::QualType
WrapperType::determineDesugaredType(clang::QualType const &Type)
{ return Type.getDesugaredType(ASTContext()); }

std::optional<TemplateArgumentList>
WrapperType::determineTemplateArgumentList(clang::QualType const &Type)
{
  auto CXXRecordDecl = Type->getAsCXXRecordDecl();
  if (!CXXRecordDecl)
    return std::nullopt;

  if (!llvm::isa<clang::ClassTemplateSpecializationDecl>(CXXRecordDecl))
    return std::nullopt;

  return TemplateArgumentList(
    llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(CXXRecordDecl)->getTemplateArgs());
}

clang::QualType const &
WrapperType::type() const
{ return Type_; }

clang::Type const *
WrapperType::typePtr() const
{ return type().getTypePtr(); }

clang::QualType
WrapperType::baseType() const
{ return getBase().type(); }

clang::Type const *
WrapperType::baseTypePtr() const
{ return baseType().getTypePtr(); }

clang::QualType
WrapperType::requalifyType(clang::QualType const &Type, unsigned Qualifiers)
{ return clang::QualType(Type.getTypePtr(), Qualifiers); }

} // namespace cppbind
