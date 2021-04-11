#include <cassert>
#include <optional>
#include <stack>
#include <string>
#include <vector>

#include "clang/AST/RecordLayout.h"

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
: Type_(determineType(Type)),
  BaseTypes_(determineBaseTypes(Type_)),
  Size_(determineSize(Type_)),
  Template_(determineTemplate(Type_)),
  TemplateArgumentList_(determineTemplateArgumentList(Type_))
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
{ return Other.str() == str(); }

bool
WrapperType::operator<(WrapperType const &Other) const
{ return str() < Other.str(); }

bool
WrapperType::isAlias() const
{ return type().getCanonicalType().getAsString() != Type_.getAsString(); }

bool
WrapperType::isTemplateInstantiation(char const *Which) const
{
  if (!Template_)
    return false;

  return !Which || (*Template_ == Which);
}

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
WrapperType::isIndirection() const
{ return isPointer() || isReference(); }

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
WrapperType::isRecordIndirection() const
{ return isIndirection() && pointee().isRecord(); }

bool
WrapperType::isConst() const
{ return type().isConstQualified(); }

WrapperType
WrapperType::canonical() const
{ return WrapperType(Type_.getCanonicalType()); }

std::vector<WrapperType>
WrapperType::baseTypes() const
{
  assert(isRecord());

  std::vector<WrapperType> BaseTypes;
  BaseTypes.reserve(BaseTypes_.size());

  for (auto const &Type : BaseTypes_)
    BaseTypes.emplace_back(Type);

  return BaseTypes;
}

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

std::size_t
WrapperType::size() const
{ return Size_; }

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
  WrapperType BaseType(fullyDerefType(Type_));

  auto Str(print::qualType(type(),
                           print::QUALIFIED_POLICY));

  auto StrBase(print::qualType(requalifyType(BaseType.type(), 0u),
                               print::QUALIFIED_POLICY));

  std::string StrReplace;

  if (WithTemplatePostfix && BaseType.isTemplateInstantiation()) {
    StrReplace = TemplateArgumentList::strip(StrBase)
               + BaseType.TemplateArgumentList_->str(true);
  } else {
    StrBase = TemplateArgumentList::strip(StrBase);
    StrReplace = StrBase;
  }

  StrReplace = Identifier(StrReplace).format(Case, Quals);

  StrReplace = WithPrefix + StrReplace + WithPostfix;

  string::replace(Str, StrBase, StrReplace);

  return Str;
}

std::string
WrapperType::mangled() const
{ return print::mangledQualType(type()); }

clang::QualType
WrapperType::determineType(clang::QualType const &Type)
{
  if (TemplateArgumentList::contains(Type.getAsString()))
    return Type.getCanonicalType();

  return Type;
}

std::vector<clang::QualType>
WrapperType::determineBaseTypes(clang::QualType const &Type)
{
  auto CXXRecordDecl = Type->getAsCXXRecordDecl();
  if (!CXXRecordDecl || !CXXRecordDecl->hasDefinition())
    return {};

  std::vector<clang::QualType> BaseTypes;

  for (auto const &Base : CXXRecordDecl->bases()) {
    if (Base.getAccessSpecifier() == clang::AS_public)
      BaseTypes.emplace_back(Base.getType());
  }

  return BaseTypes;
}

std::size_t
WrapperType::determineSize(clang::QualType const &Type)
{
  auto RecordDecl = Type->getAsRecordDecl();

  if (RecordDecl) {
    RecordDecl = RecordDecl->getDefinition();
    if (!RecordDecl)
      return 0;

    auto const &Layout = ASTContext().getASTRecordLayout(RecordDecl);

    return static_cast<std::size_t>(Layout.getSize().getQuantity());

  }

  return ASTContext().getTypeInfo(Type).Width;
}

bool
WrapperType::_isTemplateInstantiation(clang::QualType const &Type)
{
  auto CXXRecordDecl = Type->getAsCXXRecordDecl();
  if (!CXXRecordDecl)
    return false; // XXX does that cover all cases?

  return llvm::isa<clang::ClassTemplateSpecializationDecl>(CXXRecordDecl);
}

std::optional<std::string>
WrapperType::determineTemplate(clang::QualType const &Type)
{
  if (!_isTemplateInstantiation(Type))
    return std::nullopt;

  auto RD = Type->getAsCXXRecordDecl();

  return RD->getQualifiedNameAsString();
}

std::optional<TemplateArgumentList>
WrapperType::determineTemplateArgumentList(clang::QualType const &Type)
{
  if (!_isTemplateInstantiation(Type))
    return std::nullopt;

  auto RD = Type->getAsCXXRecordDecl();

  return TemplateArgumentList(
    llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(RD));
}

clang::QualType const &
WrapperType::type() const
{ return Type_; }

clang::Type const *
WrapperType::typePtr() const
{ return type().getTypePtr(); }

clang::QualType
WrapperType::fullyDerefType(clang::QualType const &Type)
{
  auto BaseType = Type.getNonReferenceType();
  while (BaseType->isPointerType())
    BaseType = BaseType->getPointeeType();

  return BaseType;
}

clang::QualType
WrapperType::requalifyType(clang::QualType const &Type, unsigned Qualifiers)
{ return clang::QualType(Type.getTypePtr(), Qualifiers); }

} // namespace cppbind
