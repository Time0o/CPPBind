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
  template<typename ...ARGS>
  void addWrapperRecord(ARGS&&... args)
  {
    WrapperRecord Wr(std::forward<ARGS>(args)...);

    if (Wr.needsImplicitDefaultConstructor())
      addWrapperFunction_(Wr.implicitDefaultConstructor());

    if (Wr.needsImplicitDestructor())
      addWrapperFunction_(Wr.implicitDestructor());

    _Records.emplace_back(std::move(Wr));
  }

  template<typename ...ARGS>
  void addWrapperFunction(ARGS&&... args)
  {
    WrapperFunction Wf(std::forward<ARGS>(args)...);
    addWrapperFunction_(Wf);
  }

  FileBuffer headerFile(std::filesystem::path const &WrappedPath) const
  {
    FileBuffer File(wrapperHeaderPath(WrappedPath));

    auto HeaderGuard(headerGuardToken(WrappedPath));

    append(File, openHeaderGuard, HeaderGuard);
    append(File, openExternCGuard, true);
    append(File, declareRecords, _Records);
    append(File, declareOrDefineFunctions, _Functions, _FunctionInsertionOrder, false);
    append(File, closeExternCGuard, true);
    append(File, closeHeaderGuard, HeaderGuard);

    return File;
  }

  FileBuffer sourceFile(std::filesystem::path const &WrappedPath) const
  {
    FileBuffer File(wrapperSourcePath(WrappedPath));

    append(File, includeHeaders, WrappedPath);
    append(File, openExternCGuard, false);
    append(File, declareOrDefineFunctions, _Functions, _FunctionInsertionOrder, true);
    append(File, closeExternCGuard, false);

    return File;
  }

private:
  static std::filesystem::path wrapperHeaderPath(
    std::filesystem::path const &WrappedPath)
  {
    return wrapperFilePath(WrappedPath,
                           WRAPPER_HEADER_OUTDIR,
                           WRAPPER_HEADER_POSTFIX,
                           WRAPPER_HEADER_EXT);
  }

  static std::filesystem::path wrapperSourcePath(
    std::filesystem::path const &WrappedPath)
  {
    return wrapperFilePath(WrappedPath,
                           WRAPPER_SOURCE_OUTDIR,
                           WRAPPER_SOURCE_POSTFIX,
                           WRAPPER_SOURCE_EXT);
  }

  static std::filesystem::path wrapperFilePath(
    std::filesystem::path const &WrappedPath,
    std::string const &Outdir,
    std::string const &Postfix,
    std::string const &Ext)
  {
    std::filesystem::path FileBuffer(WrappedPath.filename());
    std::filesystem::path WrapperDir(Outdir);

    FileBuffer.replace_filename(FileBuffer.stem().concat(Postfix));
    FileBuffer.replace_extension(Ext);

    return WrapperDir / FileBuffer;
  }

  template<typename FUNC, typename ...ARGS>
  using ReturnType = decltype(std::declval<FUNC>()(std::declval<FileBuffer &>(),
                              std::declval<ARGS>()...));

  template<typename FUNC, typename ...ARGS>
  static constexpr bool MightPutNothing =
    !std::is_same_v<ReturnType<FUNC, ARGS...>, void>;

  template<typename FUNC, typename ...ARGS>
  static std::enable_if_t<!MightPutNothing<FUNC, ARGS...>>
  append(FileBuffer &File, FUNC &&f, ARGS&&... args)
  {
    f(File, std::forward<ARGS>(args)...);
    File << FileBuffer::EmptyLine;
  }

  template<typename FUNC, typename ...ARGS>
  static std::enable_if_t<MightPutNothing<FUNC, ARGS...>>
  append(FileBuffer &File, FUNC &&f, ARGS&&... args)
  {
    if (f(File, std::forward<ARGS>(args)...))
      File << FileBuffer::EmptyLine;
  }

  static std::string headerGuardToken(std::filesystem::path const &WrappedPath)
  {
    auto Token(WRAPPER_HEADER_GUARD_PREFIX +
               WrappedPath.stem().string() +
               WRAPPER_HEADER_GUARD_POSTFIX);

    auto identifier(Identifier::makeUnqualifiedIdentifier(Token, false));

    return identifier.strUnqualified(Identifier::SNAKE_CASE_CAP_ALL);
  }

  static void openHeaderGuard(FileBuffer &File, std::string const &HeaderGuard)
  {
    File << "#ifndef " << HeaderGuard << FileBuffer::EndLine;
    File << "#define " << HeaderGuard << FileBuffer::EndLine;
  }

  static void closeHeaderGuard(FileBuffer &File, std::string const &HeaderGuard)
  { File << "#endif // " << HeaderGuard << FileBuffer::EndLine; }

  static bool openExternCGuard(FileBuffer &File, bool IfdefCpp)
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

  static bool closeExternCGuard(FileBuffer &File, bool IfdefCpp)
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

  static void includeHeaders(FileBuffer &File,
                             std::filesystem::path const &WrappedPath)
  {
    auto include = [](std::filesystem::path const &HeaderPath)
    { return "#include \"" + HeaderPath.filename().string() + "\""; };

    File << include(WrappedPath) << FileBuffer::EndLine;

    File << FileBuffer::EmptyLine;

    File << include(wrapperHeaderPath(WrappedPath)) << FileBuffer::EndLine;
  }

  static bool declareRecords(FileBuffer &File,
                             RecordsType const &Records)
  {
    if (Records.empty())
      return false;

    for (auto const &WD : Records)
      File << WD.strDeclaration() << FileBuffer::EndLine;

    return true;
  }

  static bool declareOrDefineFunctions(
    FileBuffer &File,
    FunctionsType const &Functions,
    FunctionInsertionOrderType const &FunctionInsertionOrder,
    bool IncludeBody)
  {
    if (Functions.empty())
      return false;

    auto defOrDecl = [=](WrapperFunction const &Wf, unsigned Overload = 0u)
    {
      return IncludeBody ? Wf.strDefinition(Overload)
                         : Wf.strDeclaration(Overload);
    };

    auto const &Io = FunctionInsertionOrder;
    for (auto i = 0u; i < Io.size(); ++i) {
      auto It(Functions.find(Io[i]));
      assert(It != Functions.end());

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

  void addWrapperFunction_(WrapperFunction const &Wf)
  {
    auto Name(Wf.name());

    auto It(_Functions.find(Name));

    if (It == _Functions.end())
      _FunctionInsertionOrder.push_back(Name);

    _Functions[Name].push_back(Wf);
  }

  RecordsType _Records;
  FunctionsType _Functions;
  FunctionInsertionOrderType _FunctionInsertionOrder;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_HEADER_H
