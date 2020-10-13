#ifndef GUARD_STRING_H
#define GUARD_STRING_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

namespace cppbind
{

inline std::vector<std::string> splitStr(std::string const &Str,
                                         std::string const &Delim,
                                         bool RemoveEmpty = true)
{
  if (Str.empty())
    return {};

  std::vector<std::string> Split;

  std::size_t Pos = 0u, Nextpos;

  for (;;) {
    Nextpos = Str.find(Delim, Pos);

    std::string Next;

    if (Nextpos == std::string::npos)
      Next = Str.substr(Pos, Str.size() - Pos);
    else
      Next = Str.substr(Pos, Nextpos - Pos);

    if (!Next.empty())
      Split.push_back(Next);

    if (Nextpos == std::string::npos)
      break;

    Pos = Nextpos + Delim.size();
  }

  if (RemoveEmpty) {
    for (auto it = Split.begin(); it != Split.end();) {
      if (it->empty())
        it = Split.erase(it);
      else
        ++it;
    }
  }

  return Split;
}

inline std::string pasteStrs(std::vector<std::string> const &Strs,
                             std::string const &Delim)
{
  if (Strs.empty())
    return "";

  std::stringstream SS;

  SS << Strs.front();
  for (std::size_t i = 1; i < Strs.size(); ++i)
    SS << Delim << Strs[i];

  return SS.str();
}

inline std::string transformAndPasteStrs(
  std::vector<std::string> const &Strs,
  std::string (*transform)(std::string, bool),
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

inline bool replaceStr(std::string &Str,
                       std::string const &Pat,
                       std::string const &Subst)
{
  assert(!Pat.empty());

  if (Str.empty())
    return false;

  std::size_t Start = Str.find(Pat);

  if (Start == std::string::npos)
    return false;

  Str.replace(Start, Pat.length(), Subst);

  return true;
}

inline unsigned replaceAllStrs(std::string &Str,
                               std::string const &Pat,
                               std::string const &Subst)
{
  assert(!Pat.empty());

  if (Str.empty())
    return 0u;

  unsigned Num = 0u;

  std::size_t Start = 0u;
  while ((Start = Str.find(Pat, Start)) != std::string::npos)
  {
    Str.replace(Start, Pat.length(), Subst);
    Start += Subst.length();

    ++Num;
  }

  return Num;
}

inline std::string &ltrimStr(std::string &Str)
{
  auto IT = std::find_if(
    Str.begin(),
    Str.end(),
    [](char c){ return !std::isspace<char>(c, std::locale::classic()); });

  Str.erase(Str.begin(), IT);

  return Str;
}

inline std::string &rtrimStr(std::string &Str)
{
  auto IT = std::find_if(
    Str.rbegin(),
    Str.rend(),
    [](char c){ return !std::isspace<char>(c, std::locale::classic()); });

  Str.erase(IT.base(), Str.end());

  return Str;
}

inline std::string &trimStr(std::string &Str)
{ return rtrimStr(ltrimStr(Str)); }

inline std::string &indentStr(std::string &Str)
{
  std::stringstream In(Str);
  std::stringstream Out;

  std::string Line;
  while (std::getline(In, Line, '\n'))
    Out << '\t' << ltrimStr(Line) << '\n';

  Str = Out.str();

  return Str;
}

inline bool isAllStr(std::string const &Str, char c)
{ return std::all_of(Str.begin(), Str.end(), [=](char c_){ return c_ == c; }); }

} // namespace cppbind

#endif // GUARD_STRING_H
