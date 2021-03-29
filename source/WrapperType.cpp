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
: SugaredType_(Type),
  Type_(Type.getCanonicalType()),
  BaseTypes_(determineBaseTypes(Type)),
  Size_(determineSize(Type)),
  Template_(determineTemplate(Type)),
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

std::vector<WrapperType>
WrapperType::baseTypes() const
{
  assert(isRecord());
  return BaseTypes_;
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

  if (BaseType.isRecord()) {
    auto Str(print::qualType(type()));

    auto StrBase(print::qualType(requalifyType(BaseType.type(), 0u)));

    auto StrReplace = StrBase;

    if (WithTemplatePostfix && BaseType.isTemplateInstantiation()) {
      StrReplace = TemplateArgumentList::strip(StrReplace)
                 + BaseType.TemplateArgumentList_->str(true);
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

std::optional<std::string>
WrapperType::alias() const
{
  auto TypedefType = SugaredType_->getAs<clang::TypedefType>();
  if (!TypedefType)
    return std::nullopt;

  auto Decl = TypedefType->getDecl();

  return Decl->getNameAsString();
}

std::vector<WrapperType>
WrapperType::determineBaseTypes(clang::QualType const &Type)
{
  auto CXXRecordDecl = Type->getAsCXXRecordDecl();
  if (!CXXRecordDecl || !CXXRecordDecl->hasDefinition())
    return {};

  std::vector<WrapperType> BaseTypes;

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
