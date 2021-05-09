#include <cassert>
#include <string>

#include "boost/filesystem.hpp"

#include "clang/Basic/SourceLocation.h"

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
CompilerStateRegistry::updateCompilerInstance(clang::CompilerInstance const &CI)
{ CI_ = CI; }

void
CompilerStateRegistry::updateFile(std::string const &File)
{
  TmpFile_ = File;

  auto Stem(path(File).stem().string());

  Stem = Stem.substr(0, Stem.rfind("_tmp"));

  auto It(FilesByStem_.find(Stem));
  assert(It != FilesByStem_.end());

  File_ = It->second;

  TI_->Records_.clear();
  TI_->Enums_.clear();
}

std::string
CompilerStateRegistry::currentFile(InputFile IF, bool Relative) const
{
  std::optional<std::string> File;

  switch (IF) {
  case ORIG_INPUT_FILE:
    File = File_;
    break;
  case TMP_INPUT_FILE:
    File = TmpFile_;
    break;
  default:
    break;
  }

  assert(File);

  auto Path(*File);

  if (Relative)
    Path = path(Path).filename().string();

  return Path;
}

bool
CompilerStateRegistry::inCurrentFile(InputFile IF,
                                     clang::SourceLocation const &Loc) const
{
  auto &SM(ASTContext().getSourceManager());
  auto Filename(SM.getFilename(Loc));

  if (IF == ORIG_INPUT_FILE || IF == COMPLETE_INPUT_FILE) {
    if (Filename == currentFile(ORIG_INPUT_FILE))
      return true;
  }

  if (IF == TMP_INPUT_FILE || IF == COMPLETE_INPUT_FILE) {
    if (Filename == currentFile(TMP_INPUT_FILE))
      return true;
  }

  return false;
}

clang::CompilerInstance const &
CompilerStateRegistry::operator*() const
{
  assert(CI_);
  return CI_->get();
}

clang::CompilerInstance const *
CompilerStateRegistry::operator->() const
{
  assert(CI_);
  return &CI_->get();
}

} // namespace cppbind
