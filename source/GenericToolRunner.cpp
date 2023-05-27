#include <deque>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "clang/Tooling/ArgumentsAdjusters.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "CompilerState.hpp"
#include "GenericToolRunner.hpp"
#include "Logging.hpp"
#include "Options.hpp"
#include "String.hpp"
#include "TmpFile.hpp"

namespace fs = std::filesystem;

namespace cppbind
{

GenericToolRunner::GenericToolRunner(clang::tooling::CommonOptionsParser &Parser)
: Compilations_(getCompilations(Parser)),
  SourceFiles_(getSourceFiles(Parser))
{}

int
GenericToolRunner::run()
{
  auto Tool(getTool());

  auto Factory(makeFactory());

  return Tool.run(Factory.get());
}

std::deque<TmpFile>
GenericToolRunner::getSourceFiles(clang::tooling::CommonOptionsParser &Parser)
{
  // List of input files.
  auto &SourcePathList(Parser.getSourcePathList());

  // Create temporary input files, these simply include the respective original
  // input file and can additionally contain explicit template instantiations.

  // Temporary input file directory (usually /tmp).
  auto SourceFileDirPath(fs::temp_directory_path());

  // Temporary input files.
  std::deque<TmpFile> SourceFiles;
  std::unordered_map<std::string, TmpFile *> SourceFilesByPath;
  std::unordered_map<std::string, std::string> SourcePathsByStem;

  for (auto &SourcePath_ : SourcePathList) {
    fs::path SourcePath(SourcePath_);

    auto Canonical(fs::canonical(SourcePath).string());
    auto Stem(SourcePath.stem().string());
    auto Extension(SourcePath.extension().string());

    auto PathIt(SourcePathsByStem.find(Stem));
    if (PathIt != SourcePathsByStem.end())
      throw log::exception("Source path stem '{0}' is not unique", Stem);

    auto SourcePathTmp(SourceFileDirPath / (Stem + "_tmp" + Extension));
    auto &SourceFileTmp(SourceFiles.emplace_back(SourcePathTmp));

    // Include original input file.
    SourceFileTmp << "#include \"" << Canonical << "\"\n";

    SourceFilesByPath.emplace(SourcePath.string(), &SourceFileTmp);
    SourcePathsByStem.emplace(Stem, SourcePath_);
  }

  // Append explicit template instantiations to temporary input files.
  for (auto const &TIPath_ : OPT(std::vector<std::string>, "template-instantiations")) {
    fs::path TIPath(TIPath_);

    auto Stem(TIPath.stem().string());

    auto PathIt(SourcePathsByStem.find(Stem));
    if (PathIt == SourcePathsByStem.end())
      throw log::exception("Unmatched template instantiations '{0}'", TIPath_);

    auto FileIt(SourceFilesByPath.find(PathIt->second));

    auto SourceFile(FileIt->second);

    std::ifstream TIStream(TIPath);
    if (!TIStream)
      throw log::exception("Failed to open '{0}'", TIPath_);

    (*SourceFile) << TIStream.rdbuf() << '\n';
  }

  // Pass list of source files to CompilerState.
  CompilerState().updateFileList(SourcePathList.begin(), SourcePathList.end());

  return SourceFiles;
}

clang::tooling::ClangTool
GenericToolRunner::getTool() const
{
  std::vector<std::string> SourcePathList;
  for (auto const &SourceFile : SourceFiles_)
    SourcePathList.push_back(SourceFile.path());

  clang::tooling::ClangTool Tool(Compilations_, SourcePathList);

  for (auto const &ArgumentsAdjuster : getArgumentsAdjusters())
    Tool.appendArgumentsAdjuster(ArgumentsAdjuster);

  return Tool;
}

std::vector<clang::tooling::ArgumentsAdjuster>
GenericToolRunner::getArgumentsAdjusters() const
{
  std::vector<clang::tooling::ArgumentsAdjuster> ArgumentsAdjusters;

  // Interpret all input files as C++ headers regardless of extension.
  insertArguments({"-xc++-header"}, ArgumentsAdjusters, ARGUMENTS_BEGIN);

  // Add default include directories.
  insertArguments(string::split(CLANG_INCLUDE_PATHS, " "), ArgumentsAdjusters);

  adjustArguments(ArgumentsAdjusters);

  return ArgumentsAdjusters;
}

void
GenericToolRunner::insertArguments(
  std::vector<std::string> const &Arguments,
  std::vector<clang::tooling::ArgumentsAdjuster> &ArgumentsAdjusters,
  clang::tooling::ArgumentInsertPosition Where)
{
  ArgumentsAdjusters.push_back(
    clang::tooling::getInsertArgumentAdjuster(Arguments, Where));
};

} // namespace cppbind
