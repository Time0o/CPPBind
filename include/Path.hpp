#ifndef GUARD_PATH_H
#define GUARD_PATH_H

#include <array>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>

#include "Logging.hpp"

namespace cppbind
{

namespace path
{

template<typename ...ARGS>
std::string concat(ARGS &&...Args)
{
  std::ostringstream SS;

  int Unpack[]{0, (SS << Args << "/", 0)...};
  static_cast<void>(Unpack);

  auto Path(SS.str());
  Path.pop_back();
  return Path;
}

inline std::string temporary()
{
  std::array<char, 7> Tmpnam = {'X', 'X', 'X', 'X', 'X', 'X', '\0'};

  if (mkstemp(Tmpnam.data()) == -1)
    throw exception("failed to create temporary path");

  return std::string(Tmpnam.begin(), Tmpnam.end());
}

inline void remove(std::string const &Path)
{ std::remove(Path.c_str()); }

} // namespace path

} // namespace cppbind

#endif // GUARD_PATH_H
