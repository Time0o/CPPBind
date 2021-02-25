#ifndef GUARD_IDENTIFIER_H
#define GUARD_IDENTIFIER_H

#include <string>
#include <utility>
#include <vector>

#include "clang/AST/Decl.h"
#include "clang/Basic/IdentifierTable.h"

#include "String.hpp"

namespace cppbind
{

class Identifier
{
public:
  static constexpr char const *SELF = "__self";
  static constexpr char const *RET = "__ret";
  static constexpr char const *NEW = "new";
  static constexpr char const *DELETE = "delete";

  enum Case
  {
    ORIG_CASE,
    CAMEL_CASE,
    PASCAL_CASE,
    SNAKE_CASE,
    SNAKE_CASE_CAP_FIRST,
    SNAKE_CASE_CAP_ALL
  };

  enum Quals
  {
    KEEP_QUALS,
    REMOVE_QUALS,
    REPLACE_QUALS
  };

  class Component
  {
  public:
    explicit Component(std::string const &Name);

    std::string format(Case Case = ORIG_CASE) const;

  private:
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

    static std::string stripUnderscores(std::string const &Name,
                                        std::string &LeadingUs,
                                        std::string &TrailingUs,
                                        bool &OnlyUs);

    static std::vector<std::string> splitName(std::string const &Name);

    static std::vector<std::string> splitNameSnakeCase(std::string const &Name)
    { return string::split(Name, caseDelim(SNAKE_CASE)); }

    static std::vector<std::string> splitNamePascalCase(std::string const &Name);

    static std::string caseDelim(Case Case);

    static std::string (*caseTransform(Case Case))(std::string const &, bool);

    std::string LeadingUs_;
    std::string TrailingUs_;
    bool OnlyUs_;

    std::string Name_;
    std::vector<std::string> NameWords_;
  };

  explicit Identifier(std::string const &Id);

  explicit Identifier(clang::NamedDecl const *Decl)
  : Identifier(Decl->getQualifiedNameAsString())
  {}

  static bool isIdentifier(std::string const &Name,
                           bool allowQualified = true,
                           bool allowReserved = true);

  bool operator==(Identifier const &ID) const
  { return str() == ID.str(); }

  bool operator!=(Identifier const &ID) const
  { return !(*this == ID); }

  bool operator<(Identifier const &Id) const
  { return str() < Id.str(); }

  bool operator>(Identifier const &Id) const
  { return Id < *this; }

  bool operator<=(Identifier const &Id) const
  { return !(*this > Id); }

  bool operator>=(Identifier const &Id) const
  { return !(*this < Id); }

  Identifier qualified(Identifier const &Qualifiers) const;
  Identifier unqualified() const;
  Identifier unscoped() const;

  std::string str() const;

  std::string format(Case Case = ORIG_CASE, Quals Quals = KEEP_QUALS) const;

private:
  static clang::IdentifierInfo &info(std::string const &Name);

  static bool isIdentifierChar(char c, bool first);
  static bool isKeyword(std::string const &Name);
  static bool isReserved(std::string const &Name);

  std::vector<Component> Components_;
};

inline std::size_t hash_value(Identifier const &Id)
{ return reinterpret_cast<std::size_t>(std::hash<std::string>()(Id.str())); }

} // namespace cppbind

namespace std
{

template<>
struct hash<cppbind::Identifier>
{
  std::size_t operator()(cppbind::Identifier const &Id) const
  { return cppbind::hash_value(Id); }
};

} // namespace std

#endif // GUARD_IDENTIFIER_H
