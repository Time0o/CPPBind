#ifndef GUARD_PATH_H
#define GUARD_PATH_H

#include <string>

namespace cppbind
{

constexpr char PathSep = '/';
constexpr char ExtSep = '.';

inline std::string pathFilename(std::string Path, bool WithExt = true)
{
  auto LastSlash = Path.find_last_of(PathSep);
  if (LastSlash != std::string::npos)
    Path.erase(0, LastSlash + 1);

  if (!WithExt) {
    auto LastDot = Path.find_last_of(ExtSep);
    if (LastDot != std::string::npos)
      Path.erase(LastDot);
  }

  return Path;
}

inline std::string pathDirname(std::string Path, bool WithExt = true)
{
  auto FileName(pathFilename(Path));

  auto DirName(Path.substr(0, Path.size() - FileName.size()));

  if (DirName.size() > 1 && DirName.back() == PathSep)
    DirName.pop_back();

  return DirName;
}

inline std::string pathConcat(std::string const &Path1,
                              std::string const &Path2)
{ return Path1 + PathSep + Path2; }

} // namespace cppbind

#endif // GUARD_PATH_H
