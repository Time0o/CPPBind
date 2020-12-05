#ifndef GUARD_WRAPPER_HEADER_H
#define GUARD_WRAPPER_HEADER_H

#include <cassert>
#include <filesystem>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "FileBuffer.hpp"
#include "IdentifierIndex.hpp"
#include "Logging.hpp"
#include "WrapperFunction.hpp"
#include "WrapperRecord.hpp"
#include "WrapperType.hpp"

namespace cppbind
{

class Wrapper
{
public:
  Wrapper(std::shared_ptr<IdentifierIndex> IdentifierIndex,
          std::filesystem::path const &WrappedHeader)
  : II_(IdentifierIndex),
    WrappedHeader_(WrappedHeader)
  {}

  template<typename ...ARGS>
  void addWrapperRecord(ARGS&&... args)
  {
    WrapperRecord Wr(std::forward<ARGS>(args)...);

    if (Wr.needsImplicitDefaultConstructor())
      addWrapperFunction(Wr.implicitDefaultConstructor());

    if (Wr.needsImplicitDestructor())
      addWrapperFunction(Wr.implicitDestructor());

    II_->add(Wr.name(), IdentifierIndex::TYPE);

    Records_.push_back(Wr);
  }

  template<typename ...ARGS>
  void addWrapperFunction(ARGS&&... args)
  {
    WrapperFunction Wf(std::forward<ARGS>(args)...);

    if (II_->has(Wf.name()))
      II_->pushOverload(Wf.name());
    else
      II_->add(Wf.name(), IdentifierIndex::FUNC);

    Functions_.push_back(Wf);
  }

  bool empty() const
  { return Functions_.empty(); }

  void resolveOverloads()
  {
    for (auto &Wf : Functions_)
      Wf.resolveOverload(II_);
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

    append(File, &Wrapper::openHeaderGuard);
    append(File, &Wrapper::openExternCGuard, true);
    append(File, &Wrapper::declareRecords);
    append(File, &Wrapper::declareOrDefineFunctions, false);
    append(File, &Wrapper::closeExternCGuard, true);
    append(File, &Wrapper::closeHeaderGuard);

    return File;
  }

  FileBuffer sourceFile() const
  {
    FileBuffer File;

    append(File, &Wrapper::includeHeaders);
    append(File, &Wrapper::openExternCGuard, false);
    append(File, &Wrapper::declareOrDefineFunctions, true);
    append(File, &Wrapper::closeExternCGuard, false);

    return File;
  }

  template<typename FUNC, typename ...ARGS>
  using ReturnType = decltype(
    (std::declval<Wrapper>().*std::declval<FUNC>())(
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
    auto cmp = [](WrapperType const &Wt1, WrapperType const &Wt2)
    { return Wt1.name(true) < Wt2.name(true); };

    std::set<WrapperType, decltype(cmp)> WrappableParamTypes(cmp);

    for (auto const &Wf : Functions_) {
      for (auto const &Param : Wf.params()) {
        auto ParamBaseType(Param.type().base());

        if (ParamBaseType.isWrappable(II_)) {
          WrappableParamTypes.insert(ParamBaseType.unqualifiedAndDesugared());

        } else if (!ParamBaseType.isFundamental()) {
          error() << "Parameter type "
                  << ParamBaseType.unqualified()
                  << " is not wrappable";
        }
      }
    }

    if (WrappableParamTypes.empty())
      return false;

    for (auto const &T : WrappableParamTypes)
      File << T.strDeclaration(II_) << FileBuffer::EndLine;

    return true;
  }

  bool declareOrDefineFunctions(FileBuffer &File, bool IncludeBody) const
  {
    if (Functions_.empty())
      return false;

    for (auto i = 0u; i < Functions_.size(); ++i) {
      auto &Wf(Functions_[i]);

      if (IncludeBody) {
        if (i > 0u)
          File << FileBuffer::EmptyLine;

        File << Wf.strDefinition(II_) << FileBuffer::EndLine;
      } else {
        File << Wf.strDeclaration(II_) << FileBuffer::EndLine;
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
               WrappedHeader_.stem().string() +
               WRAPPER_HEADER_GUARD_POSTFIX);

    auto identifier(Identifier::makeUnqualifiedIdentifier(Token, false));

    return identifier.strUnqualified(Identifier::SNAKE_CASE_CAP_ALL);
  }

  void includeHeaders(FileBuffer &File) const
  {
    auto include = [](std::filesystem::path const &HeaderPath)
    { return "#include \"" + HeaderPath.filename().string() + "\""; };

    File << include(WrappedHeader_) << FileBuffer::EndLine;

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
    std::filesystem::path FileBuffer(WrappedHeader_.filename());
    std::filesystem::path WrapperDir(Outdir);

    FileBuffer.replace_filename(FileBuffer.stem().concat(Postfix));
    FileBuffer.replace_extension(Ext);

    return WrapperDir / FileBuffer;
  }

  std::shared_ptr<IdentifierIndex> II_;
  std::filesystem::path WrappedHeader_;

  std::vector<WrapperRecord> Records_;
  std::vector<WrapperFunction> Functions_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_HEADER_H
