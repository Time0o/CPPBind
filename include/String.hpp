#ifndef GUARD_STRING_H
#define GUARD_STRING_H

#include <string>
#include <vector>

namespace cppbind
{

namespace string
{

bool isAll(std::string const &Str, char c);

std::string ltrim(std::string const &Str);
std::string rtrim(std::string const &Str);
std::string trim(std::string const &Str);

std::string indent(std::string const &Str);

std::vector<std::string> split(std::string const &Str,
                               std::string const &Delim,
                               bool RemoveEmpty = true);

std::string paste(std::vector<std::string> const &Strs,
                  std::string const &Delim);

std::string transformAndPaste(std::vector<std::string> const &Strs,
                              std::string (*transform)(std::string, bool),
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
