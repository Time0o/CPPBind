#include <cstdio>
#include <filesystem>

#include "TmpFile.hpp"

namespace fs = std::filesystem;

namespace cppbind
{

TmpFile::TmpFile()
// TODO: use mkstemp
: Path_(std::tmpnam(nullptr))
{}

TmpFile::TmpFile(std::string const &Path)
: Path_(Path)
{}

TmpFile::~TmpFile()
{
  if (!Removed_) {
    fs::remove(Path_);
    Removed_ = true;
  }
}

TmpFile::TmpFile(TmpFile &&Other)
{
  Other.Removed_ = true;

  Path_ = Other.Path_;
  Empty_ = Other.Empty_;
  Removed_ = false;
}

void
TmpFile::operator=(TmpFile &&Other)
{
  Other.Removed_ = true;

  Path_ = Other.Path_;
  Empty_ = Other.Empty_;
  Removed_ = false;
}

std::string
TmpFile::path(bool Relative) const
{
  if (Relative)
    return fs::path(Path_).filename().string();

  return fs::canonical(Path_).string();
}

} // namespace cppbind
