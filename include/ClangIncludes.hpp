#ifndef GUARD_BUILTIN_CLANG_INCLUDES_H
#define GUARD_BUILTIN_CLANG_INCLUDES_H

#include <sstream>
#include <string>
#include <vector>

namespace cppbind
{

static std::vector<std::string> clangIncludes()
{
  std::istringstream SS(CLANG_INCLUDE_PATHS);

  std::vector<std::string> Includes;

  std::string Inc;
  while (SS >> Inc)
    Includes.push_back(Inc);

  return Includes;
}

} // namespace cppbind

#endif // GUARD_BUILTIN_CLANG_INCLUDES_H
