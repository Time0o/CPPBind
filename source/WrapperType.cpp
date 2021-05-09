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
#include "WrapperEnum.hpp"
#include "WrapperRecord.hpp"
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

Identifier
WrapperType::getName() const
{
  if (isAlias() && isBasic())
    return Identifier(str());

  return Identifier(base().str());
}

std::optional<Identifier>
WrapperType::getNamespace() const
{
  auto TDT = type()->getAs<clang::TypedefType>();
  if (!TDT)
    return std::nullopt;

  auto TDD = TDT->getDecl();

  auto const *Context = TDD->getDeclContext();

  while (!Context->isTranslationUnit()) {
    if (Context->isNamespace()) {
      auto NamespaceDecl(llvm::dyn_cast<clang::NamespaceDecl>(Context));
      if (!NamespaceDecl->isAnonymousNamespace())
        return Identifier(NamespaceDecl);
    }

    Context = Context->getParent();
  }

  return std::nullopt;
}

std::size_t
WrapperType::getSize() const
{ return Size_; }

bool
WrapperType::isBasic() const
{ return print::qualType(type(), print::QUALIFIED_POLICY).find(' ') == std::string::npos; }

bool
WrapperType::isAlias() const
{
  return print::qualType(type(), print::QUALIFIED_POLICY) !=
         print::qualType(type().getCanonicalType(), print::QUALIFIED_POLICY);
}

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
WrapperType::isAnonymousEnum() const
{
  auto EnumType = type()->getAs<clang::EnumType>();

  if (!EnumType)
    return false;

  return EnumType->getDecl()->getDeclName().isEmpty();
}

bool
WrapperType::isIntegral() const
{
  if (!type()->isIntegralType(ASTContext()))
    return false;

  return !isBoolean() && !isScopedEnum();
}

bool
WrapperType::isSigned() const
{ return type()->isSignedIntegerType(); }

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
WrapperType::isRecordIndirection(bool Recursive) const
{ return isIndirection() && pointee(Recursive).isRecord(); }

bool
WrapperType::isConst() const
{ return type().isConstQualified(); }

WrapperType
WrapperType::base() const
{ return pointee(true).unqualified(); }

WrapperType
WrapperType::canonical() const
{ return WrapperType(type().getCanonicalType()); }

std::optional<WrapperEnum const *>
WrapperType::asEnum() const
{ return CompilerState().types()->getEnum(*this); }

std::optional<WrapperRecord const *>
WrapperType::asRecord() const
{ return CompilerState().types()->getRecord(*this); }

std::vector<std::string>
WrapperType::templateArguments() const
{
  std::vector<std::string> TArgs;
  TArgs.reserve(TemplateArgumentList_->size());

  for (auto const &TA : *TemplateArgumentList_)
    TArgs.push_back(TA.str());

  return TArgs;
}

std::optional<WrapperType>
WrapperType::proxyFor()
{
  if (!IsProxy_) {
    auto Record(CompilerState().types()->getRecord(unqualified()));
    if (!Record) {
      IsProxy_ = false;
      return std::nullopt;
    }

    std::size_t NumConstructors = (*Record)->getConstructors().size();
    std::size_t NumDefaultConstructors = !!(*Record)->getDefaultConstructor();

    if (NumConstructors - NumDefaultConstructors == 0) {
      IsProxy_ = false;
      return std::nullopt;
    }

    std::optional<WrapperType> ProxyType;

    for (auto const *Constructor : (*Record)->getConstructors()) {
      if (Constructor->isCopyConstructor() || Constructor->isMoveConstructor())
        continue;

      auto const &Params(Constructor->getParameters());

      if (Params.size() == 0)
        continue;

      if (!Constructor->isConstexpr() || Params.size() != 1) {
        IsProxy_ = false;
        return std::nullopt;
      }

      auto NextProxyType(Params[0].getType());

      if (!ProxyType) {
        ProxyType = NextProxyType;
      } else {
        if (NextProxyType.isSigned() != ProxyType->isSigned()) {
          IsProxy_ = false;
          return std::nullopt;
        }

        if (NextProxyType.getSize() > ProxyType->getSize()) {
          ProxyType = NextProxyType;
        }
      }
    }

    IsProxy_ = true;
    ProxyFor_ = ProxyType->type();
  }

  if (*IsProxy_)
    return WrapperType(*ProxyFor_);

  return std::nullopt;
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
{ return WrapperType(clang::QualType(typePtr(), Qualifiers)); }

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

std::string
WrapperType::str() const
{ return format(); }

std::string
WrapperType::format(bool WithTemplatePostfix,
                    std::string const &WithExtraPrefix,
                    std::string const &WithExtraPostfix,
                    Identifier::Case Case,
                    Identifier::Quals Quals) const
{
  std::string Str(print::qualType(type(), print::QUALIFIED_POLICY));
  std::string StrBase;
  std::string StrReplace;

  if (isAlias() && isBasic()) {
    StrBase = Str;
    StrReplace = Str;

  } else {
    WrapperType BaseType(base());

    StrBase = print::qualType(BaseType.type(), print::QUALIFIED_POLICY);

    if (WithTemplatePostfix && BaseType.isTemplateInstantiation()) {
      StrReplace = TemplateArgumentList::strip(StrBase)
                 + BaseType.templatePostfix();
    } else {
      StrBase = TemplateArgumentList::strip(StrBase);
      StrReplace = StrBase;
    }
  }

  StrReplace = Identifier(StrReplace).format(Case, Quals);

  if (!WithExtraPrefix.empty())
    StrReplace = WithExtraPrefix + StrReplace;

  if (!WithExtraPostfix.empty() &&
      StrReplace.compare(StrReplace.size() - WithExtraPostfix.size(),
                         WithExtraPostfix.size(),
                         WithExtraPostfix)) {

    StrReplace += WithExtraPostfix;
  }

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

std::string
WrapperType::templatePostfix() const
{ return TemplateArgumentList_->str(true); }

} // namespace cppbind
