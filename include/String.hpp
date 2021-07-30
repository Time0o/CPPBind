#ifndef GUARD_STRING_H
#define GUARD_STRING_H

#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace cppbind
{

// Basic string processing functions.

namespace string
{

std::string ltrim(std::string const &Str);
std::string rtrim(std::string const &Str);
std::string trim(std::string const &Str);

std::string indent(std::string const &Str);

// Split along a delimiter.
std::vector<std::string> split(std::string const &Str,
                               std::string const &Delim,
                               bool RemoveEmpty = true);

// Split along the first occurrence of a delimiter.
std::pair<std::string, std::string> splitFirst(std::string const &Str,
                                               std::string const &Delim);

// Reverse of 'split'.
std::string paste(std::vector<std::string> const &Strs,
                  std::string const &Delim);

// Replace the first occurrence of some substring.
bool replace(std::string &Str,
             std::string const &Pat,
             std::string const &Subst);

// Replace all occurrences of some substring and return their number.
unsigned replaceAll(std::string &Str,
                    std::string const &Pat,
                    std::string const &Subst);

// Remove qualifiers from a string, e.g. "foo::bar::something" => "something".
std::string removeQualifiers(std::string &Str);

} // namespace string

} // namespace cppbind

#endif // GUARD_STRING_H
