#include "Identifier.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#include "boost/algorithm/string/regex.hpp"
#include "boost/regex.hpp"

#include "clang/AST/Decl.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/Lex/Preprocessor.h"

#include "CompilerState.hpp"
#include "String.hpp"

namespace
{

template<typename T, typename FUNC>
std::string
transformAndPaste(std::vector<T> const &Strs,
                  FUNC &&transform,
                  std::string const &Delim)
{
  if (Strs.empty())
    return "";

  std::stringstream SS;

  SS << transform(Strs.front(), true);
  for (std::size_t i = 1; i < Strs.size(); ++i)
    SS << Delim << transform(Strs[i], false);

  return SS.str();
}

} // namespace

namespace cppbind
{

Identifier::Component::Component(std::string const &Name)
: Name_(stripUnderscores(Name, LeadingUs_, TrailingUs_, OnlyUs_)),
  NameWords_(splitName(Name_))
{}

std::string
Identifier::str() const
{ return format(); }

std::string
Identifier::Component::format(Case Case) const
{
  if (OnlyUs_)
    return Name_;

  std::string Str;

  if (Case == ORIG_CASE) {
    Str = Name_;
  } else {
    auto NameWordsLower(NameWords_);
    for (auto &Word : NameWordsLower)
      Word = lower(Word);

    Str = transformAndPaste(NameWordsLower,
                            caseTransform(Case),
                            caseDelim(Case));
  }

  return LeadingUs_ + Str + TrailingUs_;
}

std::string
Identifier::Component::transformStr(std::string Str, int (*transform)(int))
{
  std::transform(Str.begin(), Str.end(), Str.begin(),
                 [&](int c){ return transform(c); });
  return Str;
}

std::vector<std::string>
Identifier::Component::splitName(std::string const &Name)
{
  if (Name.size() == 1)
      return {Name};

  if (std::all_of(Name.begin(), Name.end(), [](char c){ return c == '_'; }))
    return {};

  std::vector<std::string> NameWords;
  if (Name.find(caseDelim(SNAKE_CASE)) != std::string::npos) {
    for (auto const &SnakeWord : splitNameSnakeCase(Name)) {
      auto PascalWords(splitNamePascalCase(SnakeWord));

      NameWords.insert(NameWords.end(), PascalWords.begin(), PascalWords.end());
    }
  } else {
    NameWords = splitNamePascalCase(Name);
  }

  return NameWords;
}

std::string
Identifier::Component::stripUnderscores(std::string const &Name,
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

std::vector<std::string>
Identifier::Component::splitNamePascalCase(std::string const &Name)
{
  if (Name.size() <= 2)
    return {Name};

  static boost::regex R(
    "(?<=[A-Z0-9])(?=[A-Z][a-z])"
    "|(?<=[0-9])(?=[a-z])"
    "|(?<=[a-z])(?=[A-Z0-9])"
  );

  std::vector<std::string> NameWords;

  boost::algorithm::split_regex(NameWords, Name, R);

  return NameWords;
}

std::string
Identifier::Component::caseDelim(Case Case)
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

std::string
(*Identifier::Component::caseTransform(Case Case))(std::string const &, bool)
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

Identifier::Identifier(std::string const &Id)
{
  static boost::regex RAnon("\\(anonymous .*\\)");

  auto Components(string::split(Id, "::"));

  Components_.reserve(Components.size());
  for (auto const &Component : Components) {
    if (!boost::regex_match(Component, RAnon))
      Components_.emplace_back(Component);
  }
}

bool
Identifier::isIdentifier(std::string const &Name,
                         bool allowQualified,
                         bool allowReserved)
{
  if (Name.empty())
    return false;

  if (allowQualified) {
    auto NameWords(string::split(Name, "::"));

    if (NameWords.size() > 1u && NameWords.front() == "std")
      return false;

    return std::all_of(
      NameWords.begin(),
      NameWords.end(),
      [=](std::string const &NameComponent)
      { return isIdentifier(NameComponent, false, allowReserved); });
  }

  bool isIdentifier_ =
    isIdentifierChar(Name.front(), true) &&
    std::all_of(Name.begin(),
                Name.end(),
                [](char c){ return isIdentifierChar(c, false); });

  if (!isIdentifier_)
    return false;

  if (allowReserved)
    return true;

  return !isKeyword(Name) && !isReserved(Name);
}

std::vector<Identifier>
Identifier::components() const
{
  std::vector<Identifier> Components;
  for (auto const &C : Components_)
    Components.emplace_back(C.format());

  return Components;
}

Identifier
Identifier::qualifiers() const
{
  auto Qualifiers(*this);

  Qualifiers.Components_.pop_back();

  return Qualifiers;
}

Identifier
Identifier::qualified(Identifier const &Qualifiers) const
{
  auto Qualified(Qualifiers);

  Qualified.Components_.insert(Qualified.Components_.end(),
                               Components_.begin(),
                               Components_.end());
  return Qualified;
}

Identifier
Identifier::unqualified(int Remove) const
{
  auto Unqualified(*this);

  if (Components_.size() <= 1u)
    return Unqualified;

  if (Remove == -1)
    Remove = static_cast<int>(Components_.size() - 1);
  else
    Remove = std::min(Remove, static_cast<int>(Components_.size() - 1));

  Unqualified.Components_.erase(Unqualified.Components_.begin(),
                                Unqualified.Components_.begin() + Remove);

  return Unqualified;
}

std::string
Identifier::format(Identifier::Case Case, Identifier::Quals Quals) const
{
  Identifier Id(*this);

  switch (Quals)
  {
  case KEEP_QUALS:
    break;
  case REMOVE_QUALS:
    Id = Id.unqualified();
    break;
  case REPLACE_QUALS:
    {
      auto Str(str());
      string::replaceAll(Str, "::", "_");

      Id = Identifier(Str);
    }
    break;
  }

  auto ToStr = [&](Component const &Id, bool)
               { return Id.format(Case); };

  return transformAndPaste(Id.Components_, ToStr, "::");
}

clang::IdentifierInfo &
Identifier::info(std::string const &Name)
{
  auto &IdentifierTable = CompilerState()->getPreprocessor().getIdentifierTable();

  return IdentifierTable.getOwn(Name.c_str());
}

bool
Identifier::isIdentifierChar(char c, bool first)
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

bool
Identifier::isKeyword(std::string const &Name)
{
  auto &Info(info(Name));

  return Info.isKeyword(CompilerState()->getLangOpts());
}

bool
Identifier::isReserved(std::string const &Name)
{
  auto &Info(info(Name));

  if (Info.getLength() < 2)
     return false;

  char const *NameStart = Info.getNameStart();

  char C1 = NameStart[0];
  char C2 = NameStart[1];

  return C1 == '_' && (C2 == '_' || (C2 >= 'A' && C2 <= 'Z'));
}

} // namespace cppbind
