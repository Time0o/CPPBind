#ifndef GUARD_IDENTIFIER_H
#define GUARD_IDENTIFIER_H

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#include "clang/AST/Decl.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/Lex/Preprocessor.h"

#include "llvm/ADT/StringRef.h"

#include "CompilerState.hpp"
#include "Error.hpp"
#include "Logging.hpp"
#include "String.hpp"

namespace cppbind
{

class Identifier
{
public:
  enum Case
  {
    ORIG_CASE,
    CAMEL_CASE,
    PASCAL_CASE,
    SNAKE_CASE,
    SNAKE_CASE_CAP_FIRST,
    SNAKE_CASE_CAP_ALL
  };

  explicit Identifier(std::string const &Name);

  explicit Identifier(char const *Name)
  : Identifier(std::string(Name))
  {}

  explicit Identifier(llvm::StringRef Name)
  : Identifier(Name.str())
  {}

  explicit Identifier(clang::NamedDecl const *Decl)
  : Identifier(Decl->getQualifiedNameAsString())
  {}

  static bool isIdentifier(std::string const &Name,
                           bool allowQualified = true,
                           bool allowReserved = true);

  static bool isIdentifier(llvm::StringRef Name,
                           bool allowQualified = true,
                           bool allowReserved = true)
  { return isIdentifier(Name.str(), allowQualified, allowReserved); }

  bool operator==(Identifier const &ID) const
  { return strQualified() == ID.strQualified(); }

  bool operator!=(Identifier const &ID) const
  { return !(*this == ID); }

  bool operator<(Identifier const &ID) const
  { return strQualified() < ID.strQualified(); }

  Identifier &operator+=(Identifier const &ID);

  Identifier qualify(Identifier const &Qualifiers) const
  { return Identifier(Qualifiers.strQualified() + "::" + strQualified()); }

  std::string strQualified(Case Case = ORIG_CASE,
                           bool replaceScopeResolutions = false) const;

  std::string strUnqualified(Case Case = ORIG_CASE) const;

private:
  static bool isIdentifierChar(char c, bool first);
  static bool isReservedIdentifier(clang::IdentifierInfo &Info);

  static std::string removeQuals(std::string const &Name);

  static std::string extractQuals(std::string const &Name);

  static std::string transformAndPasteComponents(
    std::vector<std::string> const &Components,
    Case Case);

  static std::string transformStr(std::string Str, int (*transform)(int));

  static std::string capitalize(std::string const &Str_)
  {
    auto Str(Str_);
    Str.front() = std::toupper(Str.front());
    return Str;
  }

  static std::string lower(std::string const &Str)
  { return transformStr(Str, std::tolower); }

  static std::string upper(std::string const &Str)
  { return transformStr(Str, std::toupper); }

  static std::string transformStrCamelCase(std::string const &Str, bool first)
  { return first ? lower(Str) : capitalize(Str); }

  static std::string transformStrPascalCase(std::string const &Str, bool)
  { return capitalize(Str); }

  static std::string transformStrSnakeCase(std::string const &Str, bool)
  { return lower(Str); }

  static std::string transformStrSnakeCaseCapFirst(std::string const &Str, bool first)
  { return first ? capitalize(Str) : lower(Str); }

  static std::string transformStrSnakeCaseCapAll(std::string const &Str, bool)
  { return upper(Str); }

  static std::vector<std::string> splitName(std::string const &Name);

  static std::string stripUnderscores(std::string const &Name,
                                      std::string &LeadingUs,
                                      std::string &TrailingUs,
                                      bool &OnlyUs);

  static std::vector<std::string> splitNameSnakeCase(std::string const &Name)
  { return string::split(Name, caseDelim(SNAKE_CASE)); }

  static std::vector<std::string> splitNamePascalCase(std::string const &Name);

  static std::string caseDelim(Case Case);

  static std::string (*caseTransform(Case Case))(std::string const &, bool);

  void assertValid() const;

  std::string LeadingUs_;
  std::string TrailingUs_;
  bool OnlyUs_;

  std::string Name_, NameQuals_;
  std::vector<std::string> NameComponents_, NameQualsComponents_;
};

inline Identifier operator+(Identifier ID1, Identifier const &ID2)
{
  ID1 += ID2;
  return ID1;
}

} // namespace cppbind

#endif // GUARD_IDENTIFIER_H
