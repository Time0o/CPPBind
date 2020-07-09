#ifndef GUARD_IDENTIFIER_H
#define GUARD_IDENTIFIER_H

#include <algorithm>
#include <cassert>
#include <cctype>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "clang/AST/Decl.h"
#include "clang/Basic/IdentifierTable.h"

#include "CompilerState.hpp"
#include "String.hpp"

namespace cppbind
{

class Identifier
{
public:
  static constexpr char const * Self = "self",
                              * New = "new",
                              * Delete = "delete";

  enum Case
  {
    ORIG_CASE,
    CAMEL_CASE,
    PASCAL_CASE,
    SNAKE_CASE,
    SNAKE_CASE_CAP_FIRST,
    SNAKE_CASE_CAP_ALL
  };

  Identifier(std::string const &Name)
  : _Name(removeQuals(Name)),
    _NameQuals(extractQuals(Name)),
    _NameComponents(splitName(_Name)),
    _NameQualsComponents(splitStr(_NameQuals, "::"))
  {
    assert(isIdentifier(_Name));

    for (auto const &NameQualsComponent : _NameQualsComponents)
      assert(isIdentifier(NameQualsComponent));
  }

  Identifier(char const *Name)
  : Identifier(std::string(Name))
  {}

  Identifier(llvm::StringRef Name)
  : Identifier(Name.str())
  {}

  Identifier(clang::NamedDecl const *Decl)
  : Identifier(Decl->getQualifiedNameAsString())
  {}

  static bool isIdentifier(std::string const &Name,
                           bool allowQualified = true,
                           bool allowReserved = true)
  {
    if (Name.empty())
      return false;

    if (allowQualified) {
      auto NameComponents(splitStr(Name, "::"));

      if (NameComponents.size() > 1u && NameComponents.front() == "std")
        return false;

      return std::all_of(
        NameComponents.begin(),
        NameComponents.end(),
        [=](std::string const &NameComponent){
          return isIdentifier(NameComponent, false, allowReserved);
        });
    }

    if (!isIdentifier_(Name))
      return false;

    if (allowReserved)
      return true;

    auto &Info(info(Name));

    return !Info.isKeyword(CompilerState()->getLangOpts()) &&
           !Info.isReservedName();
  }

  static bool isIdentifier(llvm::StringRef Name,
                           bool allowQualified = true,
                           bool allowReserved = true)
  { return isIdentifier(Name.str(), allowQualified, allowReserved); }

  static Identifier makeUnqualifiedIdentifier(std::string const &Name,
                                              bool allowReserved = false,
                                              char replaceSpecial = '_')
  {
    assert(!Name.empty());
    assert(isIdentifierChar(replaceSpecial, true));

    if (isIdentifier(Name, true, allowReserved))
      return Name;

    auto NameComponents(splitStr(Name, "::"));

    for (auto &NameComponent : NameComponents) {
      std::replace_if(NameComponent.begin(),
                      NameComponent.end(),
                      [](char c){ return !isIdentifierChar(c, false); },
                      replaceSpecial);
    }

    auto UnqualifiedIdentifier(
      pasteStrs(NameComponents, std::string(1, replaceSpecial)));

    if (!isIdentifierChar(UnqualifiedIdentifier.front(), true))
      UnqualifiedIdentifier.front() = replaceSpecial;

    if (!allowReserved) {
      if (!isIdentifier(UnqualifiedIdentifier, false, allowReserved))
        UnqualifiedIdentifier.push_back(replaceSpecial);
    }

    if (!isIdentifier(UnqualifiedIdentifier, false, allowReserved))
      throw std::runtime_error("failed to create unqualified identifier");

    return UnqualifiedIdentifier;
  }

  static Identifier makeUnqualifiedIdentifier(llvm::StringRef Name,
                                              bool allowReserved = false,
                                              char replaceSpecial = '_')
  { return makeUnqualifiedIdentifier(Name.str(), allowReserved, replaceSpecial); }

  bool operator==(Identifier const &ID) const
  { return _Name == ID._Name && _NameQuals == ID._NameQuals; }

  bool operator!=(Identifier const &ID) const
  { return !(*this == ID); }

  bool operator<(Identifier const &ID) const
  { return _NameQuals + _Name < ID._NameQuals + ID._Name; }

  Identifier &operator+=(Identifier const &ID)
  {
    _Name += ID._Name;

    _NameComponents.insert(_NameComponents.end(),
                           ID._NameComponents.begin(),
                           ID._NameComponents.end());

    return *this;
  }

  Identifier qualify(Identifier Qualifiers) const
  { return Identifier(Qualifiers.strQualified() + "::" + strQualified()); }

  std::string strQualified(
    std::variant<Case, llvm::StringRef> CaseOrStr = ORIG_CASE,
    bool replaceScopeResolutions = false) const
  {
    Case Case = unpackCase(CaseOrStr);

    if (replaceScopeResolutions) {
      assert(Case != ORIG_CASE);

      if (_NameQualsComponents.empty())
        return strUnqualified(CaseOrStr);

      return transformAndPasteComponents(_NameQualsComponents, Case) +
             caseDelim(Case) +
             strUnqualified(CaseOrStr);

    } else {
      return _NameQuals + strUnqualified(CaseOrStr);
    }
  }

  std::string strUnqualified(
    std::variant<Case, llvm::StringRef> CaseOrStr = ORIG_CASE) const
  {
    Case Case = unpackCase(CaseOrStr);

    if (Case == ORIG_CASE)
        return _Name;

    return transformAndPasteComponents(_NameComponents, Case);
  }

private:
  static std::string removeQuals(std::string const &Name)
  {
    auto pos = Name.rfind("::");

    if (pos == std::string::npos)
      return Name;

    return Name.substr(pos + 2);
  }

  static std::string extractQuals(std::string const &Name)
  {
    auto NameNoQuals(removeQuals(Name));

    return Name.substr(0, Name.size() - NameNoQuals.size());
  }

  static std::string transformAndPasteComponents(
    std::vector<std::string> const &Components,
    Case Case)
  {
    return transformAndPasteStrs(Components,
                                 caseTransform(Case),
                                 caseDelim(Case));
  }

  static std::string transformStr(std::string Str, int (*transform)(int))
  {
    std::transform(Str.begin(), Str.end(), Str.begin(),
                   [&](int c){ return transform(c); });
    return Str;
  }

  static std::string identity(std::string Str)
  { return Str; }

  static std::string capitalize(std::string Str)
  { Str.front() = std::toupper(Str.front()); return Str; }

  static std::string lower(std::string Str)
  { return transformStr(Str, std::tolower); }

  static std::string upper(std::string Str)
  { return transformStr(Str, std::toupper); }

  static std::string transformStrCamelCase(std::string Str, bool first)
  { return first ? lower(Str) : capitalize(Str); }

  static std::string transformStrPascalCase(std::string Str, bool)
  { return capitalize(Str); }

  static std::string transformStrSnakeCase(std::string Str, bool)
  { return lower(Str); }

  static std::string transformStrSnakeCaseCapFirst(std::string Str, bool first)
  { return first ? capitalize(Str) : lower(Str); }

  static std::string transformStrSnakeCaseCapAll(std::string Str, bool)
  { return upper(Str); }

  static std::vector<std::string> splitName(std::string Name)
  {
    if (Name.size() == 1)
        return {Name};

    std::string LeadingUs, TrailingUs;
    Name = stripUnderscores(Name, LeadingUs, TrailingUs);

    std::vector<std::string> NameComponents;
    if (Name.find(caseDelim(SNAKE_CASE)) != std::string::npos)
      NameComponents = splitNameSnakeCase(Name);
    else
      NameComponents = splitNamePascalCase(Name);

    NameComponents.front() = LeadingUs + NameComponents.front();
    NameComponents.back() = NameComponents.back() + TrailingUs;

    return NameComponents;
  }

  static std::string stripUnderscores(std::string const &Name,
                                      std::string &LeadingUs,
                                      std::string &TrailingUs)
  {
    std::size_t LeadingUsEnd = 0u;
    while (LeadingUsEnd < Name.size() &&
           Name[LeadingUsEnd] == '_')
      ++LeadingUsEnd;

    if (LeadingUsEnd == Name.size())
      return {Name};

    std::size_t TrailingUsBeg = Name.size() - 1u;
    while (Name[TrailingUsBeg] == '_')
      --TrailingUsBeg;

    LeadingUs = Name.substr(0u, LeadingUsEnd);
    TrailingUs = Name.substr(TrailingUsBeg + 1u);

    return Name.substr(LeadingUsEnd, TrailingUsBeg - LeadingUsEnd + 1u);
  }

  static std::vector<std::string> splitNameSnakeCase(std::string const &Name)
  { return splitStr(Name, caseDelim(SNAKE_CASE)); }

  static std::vector<std::string> splitNamePascalCase(std::string const &Name)
  {
    std::vector<std::string> NameComponents;

    std::size_t i = 0, j = 1, next;

    for (;;) {
      next = std::min(j + 1, Name.size() - 1);

      while (j < Name.size()) {
        next = std::min(j + 1, Name.size() - 1);

        if (std::isupper(Name[j]) != std::isupper(Name[next]))
          break;
        ++j;
      }

      if (j == Name.size()) {
        NameComponents.push_back(Name.substr(i, j - i));
        break;
      }

      int Offs = std::isupper(Name[j]) ? -1 : 1;

      NameComponents.push_back(Name.substr(i, j - i + Offs));

      i = j + Offs;
      j = i + 1;
    }

    return NameComponents;
  }

  static Case unpackCase(std::variant<Case, llvm::StringRef> CaseOrStr)
  {
    if (std::holds_alternative<Case>(CaseOrStr))
      return std::get<Case>(CaseOrStr);
    else
      return Options().get<Case>(std::get<llvm::StringRef>(CaseOrStr));
  }

  static std::string caseDelim(Case Case)
  {
    switch (Case) {
      case SNAKE_CASE:
      case SNAKE_CASE_CAP_FIRST:
      case SNAKE_CASE_CAP_ALL:
        return "_";
      default:
        return "";
    }
  }

  static std::string (*caseTransform(Case Case))(std::string, bool)
  {
    switch (Case) {
      case ORIG_CASE:
        return nullptr;
      case CAMEL_CASE:
        return transformStrCamelCase;
      case PASCAL_CASE:
        return transformStrPascalCase;
      case SNAKE_CASE:
        return transformStrSnakeCase;
      case SNAKE_CASE_CAP_FIRST:
        return transformStrSnakeCaseCapFirst;
      case SNAKE_CASE_CAP_ALL:
        return transformStrSnakeCaseCapAll;
    }
  }

  static bool isIdentifierChar(char c, bool first)
  {
      auto isUnderscore = [](char c){
        return c == '_';
      };

      auto isLetter = [](char c){
        return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
      };

      auto isNumber = [](char c){
        return '0' <= c && c <= '9';
      };

      return first ? isUnderscore(c) || isLetter(c)
                   : isUnderscore(c) || isLetter(c) || isNumber(c);
  }

  static bool isIdentifier_(std::string const &Name)
  {
    return isIdentifierChar(Name.front(), true) &&
           std::all_of(Name.begin(),
                       Name.end(),
                       [](char c){ return isIdentifierChar(c, false); });
  }

  static clang::IdentifierInfo &info(std::string const &Name)
  {
    auto &IdentifierTable(CompilerState()->getPreprocessor().getIdentifierTable());

    return IdentifierTable.getOwn(Name.c_str());
  }

  std::string _Name, _NameQuals;
  std::vector<std::string> _NameComponents, _NameQualsComponents;
};

Identifier operator+(Identifier ID1, Identifier const &ID2)
{
  ID1 += ID2;
  return ID1;
}

} // namespace cppbind

#endif // GUARD_IDENTIFIER_H
