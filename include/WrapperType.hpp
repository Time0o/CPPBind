#ifndef GUARD_WRAPPER_TYPE_H
#define GUARD_WRAPPER_TYPE_H

#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"

#include "FundamentalTypes.hpp"
#include "Identifier.hpp"

namespace cppbind
{

class WrapperType
{
  struct Tag
  { };

  enum TagType
  {
    UNDERLIES_ENUM_TAG
  };

  struct UnderliesEnumTag : public Tag
  {
    static constexpr TagType type = UNDERLIES_ENUM_TAG;

    clang::QualType EnumBaseType;
  };

  template<typename T>
  struct ConstructibleTag : public T
  {
    template<typename ...Args>
    ConstructibleTag(Args &&...args)
    : T{ {}, std::forward<Args>(args)... }
    {}
  };

  using TagSet = std::unordered_map<TagType, std::shared_ptr<Tag>>;

public:
  explicit WrapperType(clang::QualType const &Type)
  : WrapperType(Type, {})
  {}

  explicit WrapperType(clang::Type const *Type)
  : WrapperType(Type, {})
  {}

  explicit WrapperType(std::string const &TypeName)
  : WrapperType(TypeName, {})
  {}

  explicit WrapperType(clang::TypeDecl const *Decl)
  : WrapperType(Decl, {})
  {}

  bool operator==(WrapperType const &Wt) const
  { return type() == Wt.type(); }

  bool operator!=(WrapperType const &Wt) const
  { return !(*this == Wt); }

  bool isFundamental(char const *Which = nullptr) const;

  bool isVoid() const
  { return typePtr()->isVoidType(); }

  bool isBoolean() const
  { return isFundamental("bool"); }

  bool isEnum() const
  { return typePtr()->isEnumeralType(); }

  bool isScopedEnum() const
  { return typePtr()->isScopedEnumeralType(); }

  bool isIntegral() const;
  bool isIntegralUnderlyingEnum() const;
  bool isIntegralUnderlyingScopedEnum() const;

  bool isFloating() const
  { return typePtr()->isFloatingType(); }

  bool isReference() const
  { return typePtr()->isReferenceType(); }

  bool isLValueReference() const
  { return typePtr()->isLValueReferenceType(); }

  bool isRValueReference() const
  { return typePtr()->isRValueReferenceType(); }

  bool isPointer() const
  { return typePtr()->isPointerType(); }

  bool isClass() const
  { return typePtr()->isClassType(); }

  bool isRecord() const;

  bool isStruct() const
  { return typePtr()->isStructureType(); }

  bool isConst() const
  { return type().isConstQualified(); }

  WrapperType lvalueReferenceTo() const;
  WrapperType rvalueReferenceTo() const;
  WrapperType referenced() const;

  WrapperType pointerTo() const;
  WrapperType pointee(bool Recursive = false) const;

  WrapperType qualified(unsigned Qualifiers) const;
  WrapperType unqualified() const;

  WrapperType withConst() const;
  WrapperType withoutConst() const;

  WrapperType withEnum() const;
  WrapperType withoutEnum() const;

  WrapperType base() const;
  WrapperType changeBase(WrapperType const &NewBase) const;

  std::string str() const;

  std::string format(bool Compact = false,
                     Identifier::Case Case = Identifier::ORIG_CASE,
                     Identifier::Quals Quals = Identifier::KEEP_QUALS) const;

private:
  WrapperType(clang::QualType const &Type, TagSet const &Tags);
  WrapperType(clang::Type const *Type, TagSet const &Tags);
  WrapperType(std::string const &TypeName, TagSet const &Tags);
  WrapperType(clang::TypeDecl const *Decl, TagSet const &Tags);

  clang::QualType const &type() const
  { return Type_; }

  clang::Type const *typePtr() const
  { return type().getTypePtr(); }

  clang::QualType baseType() const
  { return base().type(); }

  clang::Type const *baseTypePtr() const
  { return baseType().getTypePtr(); }

  unsigned qualifiers() const
  { return type().getQualifiers().getAsOpaqueValue(); }

  clang::QualType requalifyType(clang::QualType const &Type,
                                unsigned Qualifiers) const
  { return clang::QualType(Type.getTypePtr(), Qualifiers); }

  template<typename TAG>
  bool hasTag() const
  { return Tags_.find(TAG::type) != Tags_.end(); }

  template<typename TAG>
  std::shared_ptr<TAG> getTag() const
  {
    auto It(Tags_.find(TAG::type));
    assert(It != Tags_.end());

    auto Tag(std::static_pointer_cast<TAG>(It->second));
    assert(Tag);

    return Tag;
  }

  template<typename TAG, typename ...ARGS>
  TagSet withTag(ARGS &&...args) const
  {
    auto TagsNew(Tags_);
    TagsNew[TAG::type] =
      std::make_shared<ConstructibleTag<TAG>>(std::forward<ARGS>(args)...);
    return TagsNew;
  }

  template<typename TAG>
  TagSet withoutTag() const
  {
    auto TagsNew(Tags_);
    TagsNew.erase(TAG::type);
    return TagsNew;
  }

  clang::QualType Type_;

  TagSet Tags_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_TYPE_H
