#ifndef GUARD_WRAPPER_H
#define GUARD_WRAPPER_H

#include <memory>
#include <utility>
#include <vector>

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
    WrapperVariable Wv(std::forward<ARGS>(args)...);

    addIdentifier(Wv);

    Variables_.push_back(Wv);
  }

  template<typename ...ARGS>
  void addWrapperFunction(ARGS&&... args)
  {
    WrapperFunction Wf(std::forward<ARGS>(args)...);

    addIdentifier(Wf);

    Functions_.push_back(Wf);
  }

  template<typename ...ARGS>
  void addWrapperRecord(ARGS&&... args)
  {
    WrapperRecord Wr(std::forward<ARGS>(args)...);

    for (auto &Wv : Wr.Variables)
      addIdentifier(Wv);

    for (auto &Wf : Wr.Functions)
      addIdentifier(Wf);

    Records_.push_back(Wr);
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

  std::vector<WrapperVariable> variables() const
  { return Variables_; }

  std::vector<WrapperRecord> records() const
  { return Records_; }

  std::vector<WrapperFunction> functions() const
  { return Functions_; }

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

  std::vector<WrapperVariable> Variables_;
  std::vector<WrapperRecord> Records_;
  std::vector<WrapperFunction> Functions_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_H
