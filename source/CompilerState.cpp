#include <cassert>
#include <string>

#include "boost/filesystem.hpp"

#include "CompilerState.hpp"

using namespace boost::filesystem;

namespace cppbind
{

void
CompilerStateRegistry::updateFile(std::string const &File)
{
  auto Stem(path(File).stem());

  auto It(FilesByStem_.find(Stem.string()));
  assert(It != FilesByStem_.end());

  File_ = It->second;
}

void
CompilerStateRegistry::updateFileEntry(std::string const &File)
{
  auto Stem(path(File).stem());

  FilesByStem_.emplace(Stem.string(), canonical(File).string());
}

std::string
CompilerStateRegistry::currentFile(bool Relative) const
{
  assert(File_);

  auto Path(*File_);

  if (Relative)
    Path = path(Path).filename().string();

  return Path;
}

} // namespace cppbind
