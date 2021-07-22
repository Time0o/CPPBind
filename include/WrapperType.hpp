#ifndef GUARD_WRAPPER_TYPE_H
#define GUARD_WRAPPER_TYPE_H

#include <cassert>
#include <deque>
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
#include "WrapperObject.hpp"

namespace cppbind
{

class WrapperEnum;
class WrapperRecord;

class WrapperType : public WrapperObject<clang::TypeDecl>
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

  Identifier getName() const;
  std::optional<Identifier> getNamespace();

  std::size_t getSize() const;

  bool isBasic() const;
  bool isAlias() const;
  bool isTemplateInstantiation(char const *Which = nullptr) const;
  bool isFundamental(char const *Which = nullptr) const;
  bool isVoid() const;
  bool isBoolean() const;
  bool isEnum() const;
  bool isScopedEnum() const;
  bool isAnonymousEnum() const;
  bool isIntegral() const;
  bool isSigned() const;
  bool isFloating() const;
  bool isCString() const;
  bool isReference() const;
  bool isLValueReference() const;
  bool isRValueReference() const;
  bool isPointer() const;
  bool isIndirection() const;
  bool isRecord() const;
  bool isRecordIndirection(bool Recursive = false) const;
  bool isStruct() const;
  bool isClass() const;
  bool isAbstract() const;
  bool isPolymorphic() const;
  bool isConst() const;

  WrapperType basic() const;
  WrapperType canonical() const;
  std::optional<WrapperType> proxyFor();
  std::deque<WrapperType> baseTypes(bool Recursive = false) const;

  WrapperEnum const * asEnum() const;
  WrapperRecord const * asRecord() const;

  WrapperType polymorphic() const;
  WrapperType nonPolymorphic() const;

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

  std::string str() const;

  std::string format(bool WithTemplatePostfix = false,
                     std::string const &WithExtraPrefix = "",
                     std::string const &WithExtraPostfix = "",
                     Identifier::Case Case = Identifier::ORIG_CASE,
                     Identifier::Quals Quals = Identifier::KEEP_QUALS) const;

  std::vector<std::string> templateArguments() const;

  std::string mangled() const;

private:
  static clang::QualType
  determineType(clang::QualType const &Type);

  static std::vector<clang::QualType>
  determineBaseTypes(clang::QualType const &Type);

  static std::size_t
  determineSize(clang::QualType const &Type);

  static bool
  _isTemplateInstantiation(clang::QualType const &Type);

  static std::optional<std::string>
  determineTemplate(clang::QualType const &Type);

  static std::optional<TemplateArgumentList>
  determineTemplateArgumentList(clang::QualType const &Type);

  clang::QualType const &type() const;
  clang::Type const *typePtr() const;

  clang::QualType Type_;
  std::vector<clang::QualType> BaseTypes_;

  std::size_t Size_;

  std::optional<bool> IsPolymorphic_;

  std::optional<bool> IsProxy_;
  std::optional<clang::QualType> ProxyFor_;

  std::optional<std::string> Template_;
  std::optional<TemplateArgumentList> TemplateArgumentList_;
};

inline std::size_t hash_value(WrapperType const &Wt)
{ return std::hash<std::string>()(Wt.str()); }

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
