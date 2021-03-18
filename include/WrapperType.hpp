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
#include "LLVMFormat.hpp"
#include "TemplateArgument.hpp"

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

  bool operator==(WrapperType const &Other) const;

  bool operator!=(WrapperType const &Other) const
  { return !operator==(Other); }

  bool operator<(WrapperType const &Other) const;

  bool operator>(WrapperType const &Other) const
  { return Other < *this; }

  bool operator<=(WrapperType const &Other) const
  { return !operator>(Other); }

  bool operator>=(WrapperType const &Other) const
  { return !operator<(Other); }

  bool isTemplateInstantiation() const;
  bool isFundamental(char const *Which = nullptr) const;
  bool isVoid() const;
  bool isBoolean() const;
  bool isEnum() const;
  bool isScopedEnum() const;
  bool isIntegral() const;
  bool isFloating() const;
  bool isCString() const;
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

  std::size_t size() const;

  std::string str(bool WithTemplatePostfix = false) const;

  std::string format(bool WithTemplatePostfix = false,
                     std::string const &WithPrefix = "",
                     std::string const &WithPostfix = "",
                     Identifier::Case Case = Identifier::ORIG_CASE,
                     Identifier::Quals Quals = Identifier::KEEP_QUALS) const;

  std::string mangled() const;

private:
  static clang::QualType
  determineDesugaredType(clang::QualType const &Type);

  static std::optional<TemplateArgumentList>
  determineTemplateArgumentList(clang::QualType const &Type);

  clang::QualType const &type() const;
  clang::Type const *typePtr() const;
  clang::QualType baseType() const;
  clang::Type const *baseTypePtr() const;

  WrapperType getBase() const;
  void setBase(WrapperType const &NewBase);

  static clang::QualType requalifyType(clang::QualType const &Type,
                                       unsigned Qualifiers);

  clang::QualType Type_;

  std::optional<TemplateArgumentList> TemplateArgumentList_;
};

inline std::size_t hash_value(WrapperType const &Wt)
{ return std::hash<std::string>()(Wt.mangled()); }

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

namespace llvm { LLVM_FORMAT_PROVIDER(cppbind::WrapperType); }

#endif // GUARD_WRAPPER_TYPE_H
