#include <cassert>
#include <string>

#include "boost/filesystem.hpp"

#include "clang/Basic/SourceLocation.h"

#include "CompilerState.hpp"

namespace fs = boost::filesystem;

namespace cppbind
{

void
CompilerStateRegistry::updateFileEntry(std::string const &File)
{
  fs::path Path(File);

  FilesByStem_.emplace(Path.stem().string(), fs::canonical(Path).string());
}

void
CompilerStateRegistry::updateCompilerInstance(clang::CompilerInstance const &CI)
{ CI_ = CI; }

void
CompilerStateRegistry::updateFile(std::string const &File)
{
  fs::path Path(File);

  auto Stem(Path.stem().string());
  Stem = Stem.substr(0, Stem.rfind("_tmp"));

  TmpFile_ = File;

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

  fs::path Path(*File);

  if (Relative)
    Path = Path.filename();

  return Path.string();
}

bool
CompilerStateRegistry::inCurrentFile(InputFile IF,
                                     clang::SourceLocation const &Loc) const
{
  auto &SM(ASTContext().getSourceManager());

  auto Filename(SM.getFilename(Loc));
  if (Filename.empty())
    return false;

  fs::path Path(Filename.str());

  auto Canonical(fs::canonical(Path).string());

  if (IF == ORIG_INPUT_FILE || IF == COMPLETE_INPUT_FILE) {
    if (Canonical == currentFile(ORIG_INPUT_FILE))
      return true;
  }

  if (IF == TMP_INPUT_FILE || IF == COMPLETE_INPUT_FILE) {
    if (Canonical == currentFile(TMP_INPUT_FILE))
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
