#include "Identifier.hpp"

namespace cppbind
{

Identifier::Identifier(std::string const &Name)
: Name_(stripUnderscores(removeQuals(Name), LeadingUs_, TrailingUs_, OnlyUs_)),
  NameQuals_(extractQuals(Name)),
  NameComponents_(splitName(Name_)),
  NameQualsComponents_(string::split(NameQuals_, "::"))
{ assertValid(); }

bool Identifier::isIdentifier(std::string const &Name,
                              bool allowQualified,
                              bool allowReserved)
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

  bool isIdentifier_ =
    isIdentifierChar(Name.front(), true) &&
    std::all_of(Name.begin(),
                Name.end(),
                [](char c){ return isIdentifierChar(c, false); });

  if (!isIdentifier_)
    return false;

  if (allowReserved)
    return true;

  auto &IdentifierTable = CompilerState()->getPreprocessor().getIdentifierTable();

  auto &Info = IdentifierTable.getOwn(Name.c_str());

  return !Info.isKeyword(CompilerState()->getLangOpts()) &&
         !isReservedIdentifier(Info);
}

Identifier &Identifier::operator+=(Identifier const &ID)
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

std::string Identifier::strQualified(Case Case, bool replaceScopeResolutions) const
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

std::string Identifier::strUnqualified(Case Case) const
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

bool Identifier::isIdentifierChar(char c, bool first)
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

bool Identifier::isReservedIdentifier(clang::IdentifierInfo &Info)
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


std::string Identifier::removeQuals(std::string const &Name)
{
  auto pos = Name.rfind("::");

  if (pos == std::string::npos)
    return Name;

  return Name.substr(pos + 2);
}

std::string Identifier::extractQuals(std::string const &Name)
{
  auto NameNoQuals(removeQuals(Name));

  return Name.substr(0, Name.size() - NameNoQuals.size());
}

std::string Identifier::transformAndPasteComponents(
  std::vector<std::string> const &Components,
  Case Case)
{
  return string::transformAndPaste(Components,
                                   caseTransform(Case),
                                   caseDelim(Case));
}

std::string Identifier::transformStr(std::string Str, int (*transform)(int))
{
  std::transform(Str.begin(), Str.end(), Str.begin(),
                 [&](int c){ return transform(c); });
  return Str;
}

std::vector<std::string> Identifier::splitName(std::string const &Name)
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

std::string Identifier::stripUnderscores(std::string const &Name,
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

std::vector<std::string> Identifier::splitNamePascalCase(std::string const &Name)
{
  std::vector<std::string> NameComponents;

  std::size_t i = 0, j = 1;

  for (;;) {
    while (j < Name.size()) {
      std::size_t next = std::min(j + 1, Name.size() - 1);

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

std::string Identifier::caseDelim(Case Case)
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

std::string (*Identifier::caseTransform(Case Case))(std::string const &, bool)
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

void Identifier::assertValid() const
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

} // namespace cppbind
