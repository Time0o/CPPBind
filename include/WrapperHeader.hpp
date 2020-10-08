#ifndef GUARD_WRAPPER_HEADER_H
#define GUARD_WRAPPER_HEADER_H

#include <cassert>
#include <filesystem>
#include <map>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "FileBuffer.hpp"
#include "WrapperFunction.hpp"
#include "WrapperRecord.hpp"

namespace cppbind
{

class WrapperHeader
{
  using RecordsType = std::vector<WrapperRecord>;
  using FunctionsType = std::map<Identifier, std::vector<WrapperFunction>>;
  using FunctionInsertionOrderType = std::vector<Identifier>;

public:
  WrapperHeader(std::filesystem::path const &WrappedHeader)
  : _WrappedHeader(WrappedHeader)
  {}

  template<typename ...ARGS>
  void addWrapperRecord(ARGS&&... args)
  {
    WrapperRecord Wr(std::forward<ARGS>(args)...);

    if (Wr.needsImplicitDefaultConstructor())
      addWrapperFunction(Wr.implicitDefaultConstructor());

    if (Wr.needsImplicitDestructor())
      addWrapperFunction(Wr.implicitDestructor());

    _Records.emplace_back(std::move(Wr));
  }

  template<typename ...ARGS>
  void addWrapperFunction(ARGS&&... args)
  {
    WrapperFunction Wf(std::forward<ARGS>(args)...);

    auto Name(Wf.name());

    auto It(_Functions.find(Name));

    if (It == _Functions.end())
      _FunctionInsertionOrder.push_back(Name);

    _Functions[Name].push_back(Wf);
  }

  void write() const
  {
    headerFile().write(headerFilePath());
    sourceFile().write(sourceFilePath());
  }

private:
  FileBuffer headerFile() const
  {
    FileBuffer File;

    append(File, &WrapperHeader::openHeaderGuard);
    append(File, &WrapperHeader::openExternCGuard, true);
    append(File, &WrapperHeader::declareRecords);
    append(File, &WrapperHeader::declareOrDefineFunctions, false);
    append(File, &WrapperHeader::closeExternCGuard, true);
    append(File, &WrapperHeader::closeHeaderGuard);

    return File;
  }

  FileBuffer sourceFile() const
  {
    FileBuffer File;

    append(File, &WrapperHeader::includeHeaders);
    append(File, &WrapperHeader::openExternCGuard, false);
    append(File, &WrapperHeader::declareOrDefineFunctions, true);
    append(File, &WrapperHeader::closeExternCGuard, false);

    return File;
  }

  template<typename FUNC, typename ...ARGS>
  using ReturnType = decltype(
    (std::declval<WrapperHeader>().*std::declval<FUNC>())(
      std::declval<FileBuffer &>(),
      std::declval<ARGS>()...));

  template<typename FUNC, typename ...ARGS>
  static constexpr bool MightPutNothing =
    !std::is_same_v<ReturnType<FUNC, ARGS...>, void>;

  template<typename FUNC, typename ...ARGS>
  std::enable_if_t<!MightPutNothing<FUNC, ARGS...>>
  append(FileBuffer &File, FUNC &&f, ARGS&&... args) const
  {
    (this->*f)(File, std::forward<ARGS>(args)...);
    File << FileBuffer::EmptyLine;
  }

  template<typename FUNC, typename ...ARGS>
  std::enable_if_t<MightPutNothing<FUNC, ARGS...>>
  append(FileBuffer &File, FUNC &&f, ARGS&&... args) const
  {
    if ((this->*f)(File, std::forward<ARGS>(args)...))
      File << FileBuffer::EmptyLine;
  }

  bool openExternCGuard(FileBuffer &File, bool IfdefCpp) const
  {
    if (WRAPPER_HEADER_OMIT_EXTERN_C)
      return false;

    if (IfdefCpp)
      File << "#ifdef __cplusplus" << FileBuffer::EndLine;

    File << "extern \"C\" {" << FileBuffer::EndLine;

    if (IfdefCpp)
      File << "#endif" << FileBuffer::EndLine;

    return true;
  }

  bool closeExternCGuard(FileBuffer &File, bool IfdefCpp) const
  {
    if (WRAPPER_HEADER_OMIT_EXTERN_C)
      return false;

    if (IfdefCpp)
      File << "#ifdef __cplusplus" << FileBuffer::EndLine;

    File << "} // extern \"C\"" << FileBuffer::EndLine;

    if (IfdefCpp)
      File << "#endif" << FileBuffer::EndLine;

    return true;
  }

  bool declareRecords(FileBuffer &File) const
  {
    if (_Records.empty())
      return false;

    for (auto const &WD : _Records)
      File << WD.strDeclaration() << FileBuffer::EndLine;

    return true;
  }

  bool declareOrDefineFunctions(FileBuffer &File, bool IncludeBody) const
  {
    if (_Functions.empty())
      return false;

    auto defOrDecl = [=](WrapperFunction const &Wf, unsigned Overload = 0u)
    {
      return IncludeBody ? Wf.strDefinition(Overload)
                         : Wf.strDeclaration(Overload);
    };

    for (auto i = 0u; i < _FunctionInsertionOrder.size(); ++i) {
      auto It(_Functions.find(_FunctionInsertionOrder[i]));
      assert(It != _Functions.end());

      auto const &Wfs(It->second);

      if (IncludeBody && i > 0u)
        File << FileBuffer::EmptyLine;

      File << defOrDecl(Wfs[0], Wfs.size() == 1u ? 0u : 1u) << FileBuffer::EndLine;

      for (unsigned o = 1u; o < Wfs.size(); ++o) {
        if (IncludeBody)
          File << FileBuffer::EmptyLine;

        File << defOrDecl(Wfs[o], o + 1u) << FileBuffer::EndLine;
      }
    }

    return true;
  }

  void openHeaderGuard(FileBuffer &File) const
  {
    File << "#ifndef " << headerGuardToken() << FileBuffer::EndLine;
    File << "#define " << headerGuardToken() << FileBuffer::EndLine;
  }

  void closeHeaderGuard(FileBuffer &File) const
  { File << "#endif // " << headerGuardToken() << FileBuffer::EndLine; }

  std::string headerGuardToken() const
  {
    auto Token(WRAPPER_HEADER_GUARD_PREFIX +
               _WrappedHeader.stem().string() +
               WRAPPER_HEADER_GUARD_POSTFIX);

    auto identifier(Identifier::makeUnqualifiedIdentifier(Token, false));

    return identifier.strUnqualified(Identifier::SNAKE_CASE_CAP_ALL);
  }

  void includeHeaders(FileBuffer &File) const
  {
    auto include = [](std::filesystem::path const &HeaderPath)
    { return "#include \"" + HeaderPath.filename().string() + "\""; };

    File << include(_WrappedHeader) << FileBuffer::EndLine;

    File << FileBuffer::EmptyLine;

    File << include(headerFilePath()) << FileBuffer::EndLine;
  }

  std::filesystem::path headerFilePath() const
  {
    return filePath(WRAPPER_HEADER_OUTDIR,
                    WRAPPER_HEADER_POSTFIX,
                    WRAPPER_HEADER_EXT);
  }

  std::filesystem::path sourceFilePath() const
  {
    return filePath(WRAPPER_SOURCE_OUTDIR,
                    WRAPPER_SOURCE_POSTFIX,
                    WRAPPER_SOURCE_EXT);
  }

  std::filesystem::path filePath(std::string const &Outdir,
                                 std::string const &Postfix,
                                 std::string const &Ext) const
  {
    std::filesystem::path FileBuffer(_WrappedHeader.filename());
    std::filesystem::path WrapperDir(Outdir);

    FileBuffer.replace_filename(FileBuffer.stem().concat(Postfix));
    FileBuffer.replace_extension(Ext);

    return WrapperDir / FileBuffer;
  }

  std::filesystem::path _WrappedHeader;

  RecordsType _Records;
  FunctionsType _Functions;
  FunctionInsertionOrderType _FunctionInsertionOrder;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_HEADER_H
