#ifndef GUARD_STRING_H
#define GUARD_STRING_H

#include <sstream>
#include <string>
#include <vector>

namespace cppbind
{

namespace string
{

class Builder
{
public:
  template<typename T>
  Builder &operator<<(T const  &Val)
  {
    SS_ << Val;
    return *this;
  }

  operator std::string() const
  { return SS_.str(); }

private:
  std::stringstream SS_;
};

std::string ltrim(std::string const &Str);
std::string rtrim(std::string const &Str);
std::string trim(std::string const &Str);
std::string indent(std::string const &Str);

std::vector<std::string> split(std::string const &Str,
                               std::string const &Delim,
                               bool RemoveEmpty = true);

std::string paste(std::vector<std::string> const &Strs,
                  std::string const &Delim);

bool replace(std::string &Str,
             std::string const &Pat,
             std::string const &Subst);

unsigned replaceAll(std::string &Str,
                    std::string const &Pat,
                    std::string const &Subst);

} // namespace string

} // namespace cppbind

#endif // GUARD_STRING_H
