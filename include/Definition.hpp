#ifndef GUARD_DEFINITION_H
#define GUARD_DEFINITION_H

#include <string>

namespace cppbind
{

class Definition
{
public:
  explicit Definition(std::string const &Name, std::string const &Value)
  : Name_(Name),
    Value_(Value)
  {}

  std::string name() const
  { return Name_; }

  std::string value() const
  { return Value_; }

private:
  std::string Name_;
  std::string Value_;
};


} // namespace cppbind

#endif // GUARD_DEFINITION_H
