#ifndef GUARD_SNIPPET_H
#define GUARD_SNIPPET_H

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "Path.hpp"
#include "Util.hpp"

namespace cppbind
{

template<typename T>
class Snippet : private util::NotCopyOrMoveable
{
public:
  ~Snippet()
  { fileRemove(); }

  static std::string str()
  {
    std::ostringstream SS;

    char const *endl = "\n\n";

    SS << T::include_() << endl;

    SS << "namespace " << T::namespace_() << " {" << endl;

    SS << T::code() << endl;

    SS << "} // namespace " << T::namespace_();

    return SS.str();
  }

  void fileCreate(std::string const &Path = "")
  {
    if (Path.empty()) {
      Path_ = path::temporary();
      Temporary_ = true;
    } else {
      Path_ = Path;
    }

    std::ofstream Stream(Path_);
    if (!Stream)
      throw std::runtime_error("failed to generate temporary file");

    Stream << str();

    Stream.flush();
  }

  void fileRemove()
  {
    if (!Removed_) {
      path::remove(Path_);
      Removed_ = true;
    }
  }

  std::string filePath() const
  { return Path_; }

private:
  std::string Path_;

  bool Temporary_ = false;
  bool Removed_ = false;
};

struct FundamentalTypesSnippet : public Snippet<FundamentalTypesSnippet>
{
  static std::string include_();
  static std::string namespace_();
  static std::string code();
};

struct TypeinfoSnippet : public Snippet<TypeinfoSnippet>
{
  static std::string include_();
  static std::string namespace_();
  static std::string code();
};

} // namespace cppbind

#endif // GUARD_SNIPPET_H
