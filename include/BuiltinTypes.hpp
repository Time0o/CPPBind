#ifndef GUARD_BUILTIN_TYPES_H
#define GUARD_BUILTIN_TYPES_H

#include <cassert>
#include <string>
#include <unordered_map>

#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/Type.h"

#include "CompilerState.hpp"

namespace cppbind
{

class BuiltinTypeRegistry
{
  friend BuiltinTypeRegistry &BuiltinTypes();

public:
  void add(clang::BuiltinType const *Type)
  {
    clang::PrintingPolicy PP(CompilerState()->getLangOpts());

    _BuiltinTypes[Type->getNameAsCString(PP)] = Type;
  }

  clang::BuiltinType const *get(std::string const &TypeName) const
  {
    auto IT(_BuiltinTypes.find(TypeName));
    assert(IT != _BuiltinTypes.end());

    return IT->second;
  }

private:
  static BuiltinTypeRegistry &instance()
  {
    static BuiltinTypeRegistry BTR;

    return BTR;
  }

  std::unordered_map<std::string, clang::BuiltinType const *> _BuiltinTypes;
};

inline BuiltinTypeRegistry &BuiltinTypes()
{ return BuiltinTypeRegistry::instance(); }

inline void addBuiltinType(clang::BuiltinType const *Type)
{ BuiltinTypes().add(Type); }

inline clang::BuiltinType const *getBuiltinType(std::string const &TypeName)
{ return BuiltinTypes().get(TypeName); }

} // namespace cppbind

#endif // GUARD_BUILTIN_TYPES_H
