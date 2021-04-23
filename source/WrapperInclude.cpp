#include <sstream>
#include <string>

#include "boost/filesystem.hpp"

#include "WrapperInclude.hpp"

namespace fs = boost::filesystem;

namespace cppbind
{

WrapperInclude::WrapperInclude(std::string const &Path, bool IsSystem)
: IsSystem_(IsSystem)
{
  try {
    Path_ = fs::canonical(Path).string();
    IsAbsolute_ = true;
  } catch (...) {
    Path_ = Path;
    IsAbsolute_ = false;
  }
}

std::string
WrapperInclude::path(bool Relative) const
{
  if (!IsAbsolute_ || !Relative)
    return Path_;

  return fs::path(Path_).filename().string();
}

std::string
WrapperInclude::str(bool Relative) const
{
  std::ostringstream SS;

  SS << "#include "
     << (IsSystem_ ? "<" : "\"")
     << path(Relative)
     << (IsSystem_ ? ">" : "\"");

  return SS.str();
}

} // namespace cppbind
