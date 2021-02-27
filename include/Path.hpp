#ifndef GUARD_PATH_H
#define GUARD_PATH_H

#include <array>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace cppbind
{

namespace path
{

inline std::string concat(std::string const &Path1,
                          std::string const &Path2)
{ return Path1 + "/" + Path2; }

inline std::string temporary()
{
  std::array<char, 7> Tmpnam = {'X', 'X', 'X', 'X', 'X', 'X', '\0'};

  if (mkstemp(Tmpnam.data()) == -1)
    throw std::runtime_error("failed to create temporary path");

  return std::string(Tmpnam.begin(), Tmpnam.end());
}

inline void remove(std::string const &Path)
{ std::remove(Path.c_str()); }

} // namespace path

} // namespace cppbind

#endif // GUARD_PATH_H
