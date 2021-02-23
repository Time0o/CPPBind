#ifndef GUARD_WRAPPER_TYPE_H
#define GUARD_WRAPPER_TYPE_H

#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"

#include "FundamentalTypes.hpp"
#include "Identifier.hpp"

namespace cppbind
{

class WrapperType
{
  friend std::size_t hash_value(WrapperType const &);

public:
  WrapperType();
  explicit WrapperType(clang::QualType const &Type);
  explicit WrapperType(clang::Type const *Type);
  explicit WrapperType(std::string const &TypeName);
  explicit WrapperType(clang::TypeDecl const *Decl);

  bool operator==(WrapperType const &Wt) const
  { return type() == Wt.type(); }

  bool operator!=(WrapperType const &Wt) const
  { return !(*this == Wt); }

  bool isMatchedBy(char const *Matcher) const;
  bool isFundamental(char const *Which = nullptr) const;
  bool isVoid() const;
  bool isBoolean() const;
  bool isEnum() const;
  bool isScopedEnum() const;
  bool isIntegral() const;
  bool isFloating() const;
  bool isReference() const;
  bool isLValueReference() const;
  bool isRValueReference() const;
  bool isPointer() const;
  bool isRecord() const;
  bool isStruct() const;
  bool isClass() const;
  bool isConst() const;

  WrapperType lvalueReferenceTo() const;
  WrapperType rvalueReferenceTo() const;
  WrapperType referenced() const;

  WrapperType pointerTo(unsigned Repeat = 0u) const;
  WrapperType pointee(bool Recursive = false) const;

  WrapperType underlyingIntegerType() const;

  unsigned qualifiers() const;
  WrapperType qualified(unsigned Qualifiers) const;
  WrapperType unqualified() const;

  WrapperType withConst() const;
  WrapperType withoutConst() const;

  WrapperType getBase() const;
  void setBase(WrapperType const &NewBase);

  std::string str() const;

  std::string format(bool Mangled = false,
                     bool Compact = false,
                     Identifier::Case Case = Identifier::ORIG_CASE,
                     Identifier::Quals Quals = Identifier::KEEP_QUALS) const;

private:
  clang::QualType const &type() const;
  clang::Type const *typePtr() const;
  clang::QualType baseType() const;
  clang::Type const *baseTypePtr() const;

  static clang::QualType requalifyType(clang::QualType const &Type,
                                       unsigned Qualifiers);

  clang::QualType Type_;
};

inline std::size_t hash_value(WrapperType const &Wt)
{ return reinterpret_cast<std::size_t>(Wt.typePtr()); }

} // namespace cppbind

namespace std
{

template<>
struct hash<cppbind::WrapperType>
{
  std::size_t operator()(cppbind::WrapperType const &Wt) const
  { return cppbind::hash_value(Wt); }
};

} // namespace std

#endif // GUARD_WRAPPER_TYPE_H
