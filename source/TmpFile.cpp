#include "boost/filesystem.hpp"

#include "TmpFile.hpp"

using namespace boost::filesystem;

namespace cppbind
{

TmpFile::TmpFile()
: Path_(boost::filesystem::unique_path().native())
{}

TmpFile::~TmpFile()
{
  if (!Removed_) {
    boost::filesystem::remove(Path_);
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

} // namespace cppbind
