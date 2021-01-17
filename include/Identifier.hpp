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
#include <vector>

#include "clang/AST/Decl.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/Lex/Preprocessor.h"

#include "CompilerState.hpp"
#include "Logging.hpp"
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
  : Name_(stripUnderscores(removeQuals(Name), LeadingUs_, TrailingUs_, OnlyUs_)),
    NameQuals_(extractQuals(Name)),
    NameComponents_(splitName(Name_)),
    NameQualsComponents_(string::split(NameQuals_, "::"))
  { assertValid(); }

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
      auto NameComponents(string::split(Name, "::"));

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
           !isReservedIdentifier(Info);
  }

  static bool isIdentifier(llvm::StringRef Name,
                           bool allowQualified = true,
                           bool allowReserved = true)
  { return isIdentifier(Name.str(), allowQualified, allowReserved); }

  static bool isReservedIdentifier(clang::IdentifierInfo &Info)
  {
#if __clang_major__ >= 10
    return Info.isReservedName();
#else
    if (Info.getLength() < 2)
       return false;

    char const *NameStart = Info.getNameStart();

    char C1 = NameStart[0];
    char C2 = NameStart[1];

    return C1 == '_' && (C2 == '_' || (C2 >= 'A' && C2 <= 'Z'));
#endif
  }

  static Identifier makeUnqualifiedIdentifier(std::string const &Name,
                                              bool allowReserved = false,
                                              char replaceSpecial = '_')
  {
    assert(!Name.empty());
    assert(isIdentifierChar(replaceSpecial, true));

    if (isIdentifier(Name, true, allowReserved))
      return Name;

    auto NameComponents(string::split(Name, "::"));

    for (auto &NameComponent : NameComponents) {
      std::replace_if(NameComponent.begin(),
                      NameComponent.end(),
                      [](char c){ return !isIdentifierChar(c, false); },
                      replaceSpecial);
    }

    auto UnqualifiedIdentifier(
      string::paste(NameComponents, std::string(1, replaceSpecial)));

    if (!isIdentifierChar(UnqualifiedIdentifier.front(), true))
      UnqualifiedIdentifier.front() = replaceSpecial;

    if (!allowReserved) {
      if (!isIdentifier(UnqualifiedIdentifier, false, allowReserved))
        UnqualifiedIdentifier.push_back(replaceSpecial);
    }

    if (!isIdentifier(UnqualifiedIdentifier, false, allowReserved))
      error() << "failed to create unqualified identifier";

    return UnqualifiedIdentifier;
  }

  static Identifier makeUnqualifiedIdentifier(llvm::StringRef Name,
                                              bool allowReserved = false,
                                              char replaceSpecial = '_')
  { return makeUnqualifiedIdentifier(Name.str(), allowReserved, replaceSpecial); }

  bool operator==(Identifier const &ID) const
  { return strQualified() == ID.strQualified(); }

  bool operator!=(Identifier const &ID) const
  { return !(*this == ID); }

  bool operator<(Identifier const &ID) const
  { return strQualified() < ID.strQualified(); }

  Identifier &operator+=(Identifier const &ID)
  {
    if (OnlyUs_ && ID.OnlyUs_) {
      Name_ += ID.Name_;

    } else if (OnlyUs_) {
      LeadingUs_ = Name_ + ID.LeadingUs_;

      Name_ = ID.Name_;
      NameComponents_ = ID.NameComponents_;

      TrailingUs_ = ID.TrailingUs_;

      OnlyUs_ = false;

    } else if (ID.OnlyUs_) {
      TrailingUs_ += ID.Name_;

    } else {
      Name_ += ID.Name_;

      NameComponents_.insert(NameComponents_.end(),
                             ID.NameComponents_.begin(),
                             ID.NameComponents_.end());

      TrailingUs_ = ID.TrailingUs_;
    }

    assertValid();

    return *this;
  }

  Identifier qualify(Identifier Qualifiers) const
  { return Identifier(Qualifiers.strQualified() + "::" + strQualified()); }

  std::string strQualified(Case Case = ORIG_CASE,
                           bool replaceScopeResolutions = false) const
  {
    if (replaceScopeResolutions) {
      assert(Case != ORIG_CASE);

      if (NameQualsComponents_.empty())
        return strUnqualified(Case);

      return transformAndPasteComponents(NameQualsComponents_, Case) +
             caseDelim(Case) +
             strUnqualified(Case);

    } else {
      return NameQuals_ + strUnqualified(Case);
    }
  }

  std::string strUnqualified(Case Case = ORIG_CASE) const
  {
    if (OnlyUs_)
      return Name_;

    std::string Str;

    if (Case == ORIG_CASE)
      Str = Name_;
    else
      Str = transformAndPasteComponents(NameComponents_, Case);

    return LeadingUs_ + Str + TrailingUs_;
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
    return string::transformAndPaste(Components,
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

    if (string::isAll(Name, '_'))
      return {};

    std::vector<std::string> NameComponents;
    if (Name.find(caseDelim(SNAKE_CASE)) != std::string::npos)
      NameComponents = splitNameSnakeCase(Name);
    else
      NameComponents = splitNamePascalCase(Name);

    return NameComponents;
  }

  static std::string stripUnderscores(std::string const &Name,
                                      std::string &LeadingUs,
                                      std::string &TrailingUs,
                                      bool &OnlyUs)
  {
    std::size_t LeadingUsEnd = 0u;
    while (LeadingUsEnd < Name.size() && Name[LeadingUsEnd] == '_')
      ++LeadingUsEnd;

    if (LeadingUsEnd == Name.size()) {
      OnlyUs = true;
      return Name;
    } else {
      OnlyUs = false;
    }

    std::size_t TrailingUsBeg = Name.size() - 1u;
    while (Name[TrailingUsBeg] == '_')
      --TrailingUsBeg;

    LeadingUs = Name.substr(0u, LeadingUsEnd);
    TrailingUs = Name.substr(TrailingUsBeg + 1u);

    return Name.substr(LeadingUsEnd, TrailingUsBeg - LeadingUsEnd + 1u);
  }

  static std::vector<std::string> splitNameSnakeCase(std::string const &Name)
  { return string::split(Name, caseDelim(SNAKE_CASE)); }

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

  void assertValid() const
  {
#ifndef NDEBUG
    assert(!Name_.empty());

    if (OnlyUs_) {
      assert(string::isAll(Name_, '_'));

      assert(LeadingUs_.empty());
      assert(TrailingUs_.empty());

    } else {
      assert(isIdentifier(LeadingUs_ + Name_ + TrailingUs_));
      assert(Name_.front() != '_');
      assert(Name_.back() != '_');

      assert(string::isAll(LeadingUs_, '_'));
      assert(string::isAll(TrailingUs_, '_'));
    }

    for (auto const &NameQualsComponent : NameQualsComponents_)
      assert(isIdentifier(NameQualsComponent));
#endif
  }

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
