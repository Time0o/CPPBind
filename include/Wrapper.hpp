#ifndef GUARD_WRAPPER_H
#define GUARD_WRAPPER_H

#include <deque>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "Include.hpp"
#include "Logging.hpp"
#include "Options.hpp"
#include "TypeIndex.hpp"
#include "WrapperConstant.hpp"
#include "WrapperFunction.hpp"
#include "WrapperRecord.hpp"
#include "WrapperType.hpp"

namespace cppbind
{

class Wrapper
{
public:
  Wrapper(std::shared_ptr<IdentifierIndex> II,
          std::shared_ptr<TypeIndex> TI)
  : II_(II),
    TI_(TI)
  {}

  template<typename IT>
  void addIncludes(IT First, IT Last)
  {
    for (auto It = First; It != Last; ++It)
      Includes_.push_back(*It);
  }

  template<typename IT>
  void addDefinitions(IT First, IT Last)
  {
    for (auto It = First; It != Last; ++It)
      Constants_.emplace_back(Identifier(It->name()));
  }

  template<typename ...ARGS>
  void addWrapperConstant(ARGS &&...Args)
  {
    addWrapperObject(&Wrapper::_addWrapperConstant,
                     Constants_,
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

  std::deque<Include> getIncludes() const
  { return Includes_; }

  std::vector<WrapperConstant const *> getConstants() const;

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

  bool _addWrapperConstant(WrapperConstant *Constant);
  bool _addWrapperFunction(WrapperFunction *Function);
  bool _addWrapperRecord(WrapperRecord *Record);

  bool typeWrapped(WrapperType const &Type) const;
  bool checkTypeWrapped(WrapperType const &Type) const;

  std::deque<Include> Includes_;

  std::deque<WrapperConstant> Constants_;

  std::deque<WrapperFunction> Functions_;
  std::unordered_map<Identifier, std::deque<WrapperFunction const *>> FunctionNames_;

  std::deque<WrapperRecord> Records_;
  std::unordered_map<std::string, WrapperRecord const *> RecordTypesMangled_;

  std::shared_ptr<IdentifierIndex> II_;
  std::shared_ptr<TypeIndex> TI_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_H
