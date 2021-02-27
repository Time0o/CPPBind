#ifndef GUARD_STRING_H
#define GUARD_STRING_H

#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace cppbind
{

namespace string
{

std::string ltrim(std::string const &Str);
std::string rtrim(std::string const &Str);
std::string trim(std::string const &Str);
std::string indent(std::string const &Str);

std::vector<std::string> split(std::string const &Str,
                               std::string const &Delim,
                               bool RemoveEmpty = true);

std::pair<std::string, std::string> splitFirst(std::string const &Str,
                                               std::string const &Delim);

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
