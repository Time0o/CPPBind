#include <cassert>
#include <string>

#include "boost/filesystem.hpp"

#include "CompilerState.hpp"

using namespace boost::filesystem;

namespace cppbind
{

void
CompilerStateRegistry::updateFileEntry(std::string const &File)
{
  auto Stem(path(File).stem());

  FilesByStem_.emplace(Stem.string(), canonical(File).string());
}

void
CompilerStateRegistry::updateFile(std::string const &File)
{
  TmpFile_ = File;

  auto Stem(path(File).stem().string());

  Stem = Stem.substr(0, Stem.rfind("_tmp"));

  auto It(FilesByStem_.find(Stem));
  assert(It != FilesByStem_.end());

  File_ = It->second;
}

std::string
CompilerStateRegistry::currentFile(bool Tmp, bool Relative) const
{
  auto File(Tmp ? TmpFile_ : File_);

  assert(File);

  auto Path(*File);

  if (Relative)
    Path = path(Path).filename().string();

  return Path;
}

} // namespace cppbind
