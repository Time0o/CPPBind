#include <algorithm>
#include <cassert>
#include <locale>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "String.hpp"

namespace cppbind
{

namespace string
{

std::string ltrim(std::string const &Str_)
{
  auto Str(Str_);

  auto IT = std::find_if(
    Str.begin(),
    Str.end(),
    [](char c){ return !std::isspace<char>(c, std::locale::classic()); });

  Str.erase(Str.begin(), IT);

  return Str;
}

std::string rtrim(std::string const &Str_)
{
  auto Str(Str_);

  auto IT = std::find_if(
    Str.rbegin(),
    Str.rend(),
    [](char c){ return !std::isspace<char>(c, std::locale::classic()); });

  Str.erase(IT.base(), Str.end());

  return Str;
}

std::string trim(std::string const &Str)
{ return rtrim(ltrim(Str)); }

std::vector<std::string> split(std::string const &Str,
                               std::string const &Delim,
                               bool RemoveEmpty)
{
  if (Str.empty())
    return {};

  std::vector<std::string> Split;

  std::size_t Pos = 0u;

  for (;;) {
    std::size_t Nextpos = Str.find(Delim, Pos);

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

std::pair<std::string, std::string> splitFirst(std::string const &Str,
                                               std::string const &Delim)
{
  std::size_t Pos = Str.find(Delim);

  if (Pos == std::string::npos)
    return std::make_pair(Str, "");

  return std::make_pair(Str.substr(0, Pos), Str.substr(Pos + Delim.size()));
}

std::string paste(std::vector<std::string> const &Strs,
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

bool replace(std::string &Str,
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

unsigned replaceAll(std::string &Str,
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

std::string indent(std::string const &Str)
{
  std::stringstream In(Str);
  std::stringstream Out;

  std::string Line;
  while (std::getline(In, Line, '\n'))
    Out << '\t' << ltrim(Line) << '\n';

  return Out.str();
}

} // namespace string

} // namespace cppbind
