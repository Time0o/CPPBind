#ifndef GUARD_WRAPPER_H
#define GUARD_WRAPPER_H

#include <deque>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Identifier.hpp"
#include "Logging.hpp"
#include "Options.hpp"
#include "WrapperDefinition.hpp"
#include "WrapperEnum.hpp"
#include "WrapperFunction.hpp"
#include "WrapperInclude.hpp"
#include "WrapperRecord.hpp"
#include "WrapperType.hpp"
#include "WrapperVariable.hpp"

namespace cppbind
{

class Wrapper
{
public:
  template<typename ...ARGS>
  void addInclude(ARGS &&...Args)
  { Includes_.emplace_back(std::forward<ARGS>(Args)...); }

  template<typename ...ARGS>
  void addDefinition(ARGS &&...Args)
  { Definitions_.emplace_back(std::forward<ARGS>(Args)...); }

  template<typename ...ARGS>
  void addWrapperEnum(ARGS &&...Args)
  {
    addWrapperObject(&Wrapper::_addWrapperEnum,
                     Enums_,
                     std::forward<ARGS>(Args)...);
  }

  template<typename ...ARGS>
  void addWrapperVariable(ARGS &&...Args)
  {
    addWrapperObject(&Wrapper::_addWrapperVariable,
                     Variables_,
                     std::forward<ARGS>(Args)...);
  }

  template<typename ...ARGS>
  void addWrapperFunction(ARGS &&...Args)
  {
    addWrapperObject(&Wrapper::_addWrapperFunction,
                     Functions_,
                     std::forward<ARGS>(Args)...);
  }

  template<typename ...ARGS>
  void addWrapperRecord(ARGS &&...Args)
  {
    addWrapperObject(&Wrapper::_addWrapperRecord,
                     Records_,
                     std::forward<ARGS>(Args)...);
  }

  void overload();

  std::vector<WrapperInclude const *> getIncludes() const;
  std::vector<WrapperDefinition const *> getDefinitions() const;
  std::vector<WrapperEnum const *> getEnums() const;
  std::vector<WrapperVariable const *> getVariables() const;
  std::vector<WrapperFunction const *> getFunctions() const;
  std::vector<WrapperRecord const *> getRecords() const;

private:
  template<typename T, typename ...ARGS>
  void addWrapperObject(bool(Wrapper::*addObj)(T *),
                        std::deque<T> &Objs,
                        ARGS &&...Args)
  {
    Objs.emplace_back(std::forward<ARGS>(Args)...);

    log::info("creating {0}...", Objs.back());

    if (!(this->*addObj)(&Objs.back()))
      Objs.pop_back();
    else
      log::info("created {0}", Objs.back());
  }

  bool _addWrapperEnum(WrapperEnum *Enum);
  bool _addWrapperVariable(WrapperVariable *Variable);
  bool _addWrapperFunction(WrapperFunction *Function);
  bool _addWrapperRecord(WrapperRecord *Record);

  bool typeWrapped(WrapperType const &Type) const;
  bool checkTypeWrapped(WrapperType const &Type) const;

  std::deque<WrapperInclude> Includes_;
  std::deque<WrapperDefinition> Definitions_;

  std::deque<WrapperEnum> Enums_;
  std::deque<WrapperVariable> Variables_;

  std::deque<WrapperFunction> Functions_;
  std::unordered_map<Identifier, std::deque<WrapperFunction const *>> FunctionNames_;

  std::deque<WrapperRecord> Records_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_H
