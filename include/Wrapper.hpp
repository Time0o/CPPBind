#ifndef GUARD_WRAPPER_H
#define GUARD_WRAPPER_H

#include <deque>
#include <memory>
#include <utility>

#include "IdentifierIndex.hpp"
#include "WrapperVariable.hpp"
#include "WrapperFunction.hpp"
#include "WrapperRecord.hpp"

namespace cppbind
{

class Wrapper
{
public:
  explicit Wrapper(std::shared_ptr<IdentifierIndex> IdentifierIndex,
                   std::string const &InputFile)
  : II_(IdentifierIndex),
    InputFile_(InputFile)
  {}

  template<typename ...ARGS>
  void addWrapperVariable(ARGS&&... args)
  {
    Variables_.emplace_back(std::forward<ARGS>(args)...);

    addIdentifier(Variables_.back());
  }

  template<typename ...ARGS>
  void addWrapperFunction(ARGS&&... args)
  {
    Functions_.emplace_back(std::forward<ARGS>(args)...);

    addIdentifier(Functions_.back());
  }

  template<typename ...ARGS>
  void addWrapperRecord(ARGS&&... args)
  {
    Records_.emplace_back(std::forward<ARGS>(args)...);

    for (auto &Wv : Records_.back().Variables)
      addIdentifier(Wv);

    for (auto &Wf : Records_.back().Functions)
      addIdentifier(Wf);
  }

  void overload()
  {
    for (auto &Wr : Records_) {
      for (auto &Wf : Wr.Functions)
        overloadIdentifier(Wf);
    }

    for (auto &Wf : Functions_)
      overloadIdentifier(Wf);
  }

  std::string inputFile() const
  { return InputFile_; }

  std::deque<WrapperVariable> const *variables() const
  { return &Variables_; }

  std::deque<WrapperRecord> const *records() const
  { return &Records_; }

  std::deque<WrapperFunction> const *functions() const
  { return &Functions_; }

private:
  void addIdentifier(WrapperVariable &Wv)
  { Wv.setName(II_->add(Wv.getName(), IdentifierIndex::CONST)); }

  void addIdentifier(WrapperFunction &Wf)
  {
    if (II_->has(Wf.getName()))
      II_->pushOverload(Wf.getName());
    else
      Wf.setName(II_->add(Wf.getName(), IdentifierIndex::FUNC));
  }

  void overloadIdentifier(WrapperFunction &Wf)
  {
    if (!II_->hasOverload(Wf.getName()))
      return;

    Wf.overload(II_->popOverload(Wf.getName()));
    Wf.setNameOverloaded(II_->add(Wf.getNameOverloaded(), IdentifierIndex::FUNC));
  }

  std::shared_ptr<IdentifierIndex> II_;

  std::string InputFile_;

  std::deque<WrapperVariable> Variables_;
  std::deque<WrapperRecord> Records_;
  std::deque<WrapperFunction> Functions_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_H
