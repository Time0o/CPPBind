#ifndef GUARD_TYPEINFO_H
#define GUARD_TYPEINFO_H

#include <sstream>
#include <string>

#include "WrapperRecord.hpp"

namespace cppbind
{

struct Typeinfo
{
  static constexpr char const *Namespace = "__typeinfo";

  static std::string typeInstances()
  {
    std::ostringstream SS;

    auto unmangledTypename = [](WrapperRecord const *Wr)
    { return Wr->getType().str(); };

    auto mangledTypename = [](WrapperRecord const *Wr)
    { return Wr->getType().format(true); };

    for (auto const *Wr :
         WrapperRecord::ordering(WrapperRecord::PARENTS_FIRST_ORDERING)) {

      SS << "type_instance<" << unmangledTypename(Wr);

      if (!Wr->getParents().empty()) {
        for (auto const *Pr : Wr->getParents())
          SS << ", " << unmangledTypename(Pr);
      }

      SS << "> " << mangledTypename(Wr) << ";\n";
    }

    return SS.str();
  }

  static std::string makeTypedPtr(std::string const &Arg)
  { return Namespace + ("::make_typed_ptr(" + Arg + ")"); }

  static std::string typedPtrCast(WrapperType const &Type, std::string const &Arg)
  { return Namespace + ("::typed_ptr_cast<" + Type.str() + ">(" + Arg + ")"); }
};

} // namespace cppbind

#endif // GUARD_TYPEINFO_H
