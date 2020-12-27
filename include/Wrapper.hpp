#ifndef GUARD_WRAPPER_HEADER_H
#define GUARD_WRAPPER_HEADER_H

#include <cassert>
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

class Wrapper;

class WrapperInclude
{
public:
  enum IncludeIn
  {
    HEADER,
    SOURCE,
    BOTH
  };

  WrapperInclude(std::string const &Header, bool System, IncludeIn In)
  : Header_(Header),
    System_(System),
    In_(In)
  {}

  std::string header() const
  { return Header_; }

  bool system() const
  { return System_; }

  bool inHeader() const
  { return In_ == HEADER || In_ == BOTH; }

  bool inSource() const
  { return In_ == SOURCE || In_ == BOTH; }

private:
  std::string Header_;
  bool System_;
  IncludeIn In_;
};

class WrapperFiles
{
  friend class Wrapper;

public:
  WrapperFiles(bool Boilerplate = true)
  : Boilerplate_(Boilerplate)
  {}

  bool empty() const
  { return HeaderFile_.empty() && SourceFile_.empty(); }

  FileBuffer const &header() const
  { return HeaderFile_; }

  FileBuffer const &source() const
  { return SourceFile_; }

  std::vector<WrapperInclude> includes() const
  { return Includes_; }

  void write() const
  {
    HeaderFile_.write(HeaderFilePath_);
    SourceFile_.write(SourceFilePath_);
  }

private:
  bool boilerplate() const
  { return Boilerplate_; }

  void addHeader(FileBuffer &&File, std::string const &Path)
  {
    HeaderFile_ = std::move(File);
    HeaderFilePath_ = Path;
  }

  void addSource(FileBuffer &&File, std::string const &Path)
  {
    SourceFile_ = std::move(File);
    SourceFilePath_ = Path;
  }

  void addIncludes(std::vector<WrapperInclude> const &Includes)
  { Includes_ = Includes; }

  bool Boilerplate_;

  FileBuffer HeaderFile_;
  std::string HeaderFilePath_;

  FileBuffer SourceFile_;
  std::string SourceFilePath_;

  std::vector<WrapperInclude> Includes_;
};

class Wrapper
{
public:
  Wrapper(std::shared_ptr<IdentifierIndex> IdentifierIndex,
          std::string const &WrappedHeader)
  : II_(IdentifierIndex),
    WrappedHeader_(WrappedHeader)
  {
    Includes_.emplace_back(WrappedHeader_, false, WrapperInclude::SOURCE);
    Includes_.emplace_back(headerFilePath(), false, WrapperInclude::SOURCE);
  }

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

    if (Wf.hasDefaultParams()) {
      Wf.removeDefaultParam();
      addWrapperFunction(Wf);
    } else {
      for (auto const &Type : Wf.types()) {
        auto CHeader(Type.inCHeader());

        if (CHeader)
          addInclude(*CHeader, true, WrapperInclude::BOTH);

        if (Type.isRValueReference())
          addInclude("utility", true, WrapperInclude::SOURCE);
      }
    }
  }

  template<typename ...ARGS>
  void addInclude(ARGS&&... args)
  { Includes_.emplace_back(std::forward<ARGS>(args)...); }

  bool empty() const
  { return Functions_.empty(); }

  void overload()
  {
    for (auto &Wf : Functions_)
      Wf.overload(II_);
  }

  void write(std::shared_ptr<WrapperFiles> Files)
  {
    Files->addHeader(headerFile(Files->boilerplate()), headerFilePath());
    Files->addSource(sourceFile(Files->boilerplate()), sourceFilePath());

    Files->addIncludes(Includes_);
  }

private:
  FileBuffer headerFile(bool Boilerplate = false) const
  {
    FileBuffer File;

    if (Boilerplate) {
      append(File, &Wrapper::openHeaderGuard);
      append(File, &Wrapper::openExternCGuard, false);
      append(File, &Wrapper::includeHeaders, false);
      append(File, &Wrapper::declareRecords);
    }

    append(File, &Wrapper::declareOrDefineFunctions, false);

    if (Boilerplate) {
      append(File, &Wrapper::closeExternCGuard, false);
      append(File, &Wrapper::closeHeaderGuard);
    }

    return File;
  }

  std::string headerFilePath() const
  {
    return wrapperFilePath(WRAPPER_HEADER_OUTDIR,
                           WRAPPER_HEADER_POSTFIX,
                           WRAPPER_HEADER_EXT);
  }

  FileBuffer sourceFile(bool Boilerplate = false) const
  {
    FileBuffer File;

    if (Boilerplate) {
      append(File, &Wrapper::includeHeaders, true);
      append(File, &Wrapper::openExternCGuard, true);
    }

    append(File, &Wrapper::declareOrDefineFunctions, true);

    if (Boilerplate) {
      append(File, &Wrapper::closeExternCGuard, true);
    }

    return File;
  }

  std::string sourceFilePath() const
  {
    return wrapperFilePath(WRAPPER_SOURCE_OUTDIR,
                           WRAPPER_SOURCE_POSTFIX,
                           WRAPPER_SOURCE_EXT);
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

  bool openExternCGuard(FileBuffer &File, bool Source) const
  {
    if (WRAPPER_HEADER_OMIT_EXTERN_C)
      return false;

    if (!Source)
      File << "#ifdef __cplusplus" << FileBuffer::EndLine;

    File << "extern \"C\" {" << FileBuffer::EndLine;

    if (!Source)
      File << "#endif" << FileBuffer::EndLine;

    return true;
  }

  bool closeExternCGuard(FileBuffer &File, bool Source) const
  {
    if (WRAPPER_HEADER_OMIT_EXTERN_C)
      return false;

    if (!Source)
      File << "#ifdef __cplusplus" << FileBuffer::EndLine;

    File << "} // extern \"C\"" << FileBuffer::EndLine;

    if (!Source)
      File << "#endif" << FileBuffer::EndLine;

    return true;
  }

  bool declareRecords(FileBuffer &File) const
  {
    auto cmp = [](WrapperType const &Wt1, WrapperType const &Wt2)
    { return Wt1.name() < Wt2.name(); };

    std::set<WrapperType, decltype(cmp)> WrappableParamTypes(cmp);

    for (auto const &Wf : Functions_) {
      for (auto const &Param : Wf.params()) {
        auto ParamBaseType(Param.type().base());

        if (ParamBaseType.isWrappable(II_)) {
          WrappableParamTypes.insert(ParamBaseType.unqualified());

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

  bool declareOrDefineFunctions(FileBuffer &File, bool Source) const
  {
    if (Functions_.empty())
      return false;

    for (auto i = 0u; i < Functions_.size(); ++i) {
      auto &Wf(Functions_[i]);

      if (Source) {
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
               pathFilename(WrappedHeader_, false) +
               WRAPPER_HEADER_GUARD_POSTFIX);

    auto identifier(Identifier::makeUnqualifiedIdentifier(Token, false));

    return identifier.strUnqualified(Identifier::SNAKE_CASE_CAP_ALL);
  }

  bool includeHeaders(FileBuffer &File, bool Source) const
  {
    auto include = [](std::string const &Header, bool System)
    {
      std::stringstream SS;
      SS << "#include ";

      if (System)
        SS << "\"" << pathFilename(Header) << "\"";
      else
        SS << "<" << Header << ">";

      return SS.str();
    };

    bool NoIncludes = true;
    for (auto const &Include : Includes_) {
      if ((Source && Include.inSource()) || (!Source && Include.inHeader())) {
        File << include(Include.header(), Include.system()) << FileBuffer::EndLine;
        NoIncludes = false;
      }
    }

    return !NoIncludes;
  }

  static constexpr char PathSep = '/';
  static constexpr char ExtSep = '.';

  std::string wrapperFilePath(std::string const &Outdir,
                              std::string const &Postfix,
                              std::string const &Ext) const
  {
    auto Filename(pathFilename(WrappedHeader_, false) + Postfix + ExtSep + Ext);

    return pathConcat(Outdir, Filename);
  }

  static std::string pathFilename(std::string Path, bool WithExt = true)
  {
    auto LastSlash = Path.find_last_of(PathSep);
    if (LastSlash != std::string::npos)
      Path.erase(0, LastSlash + 1);

    if (!WithExt) {
      auto LastDot = Path.find_last_of(ExtSep);
      if (LastDot != std::string::npos)
        Path.erase(LastDot);
    }

    return Path;
  }

  static std::string pathConcat(std::string const &Path1,
                                std::string const &Path2)
  { return Path1 + PathSep + Path2; }

  std::shared_ptr<IdentifierIndex> II_;
  std::string WrappedHeader_;

  std::vector<WrapperRecord> Records_;
  std::vector<WrapperFunction> Functions_;
  std::vector<WrapperInclude> Includes_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_HEADER_H
