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
                   std::string const &WrappedFile)
  : II_(IdentifierIndex),
    WrappedFile_(WrappedFile)
  {}

  template<typename ...ARGS>
  void addWrapperVariable(ARGS&&... args)
  {
    WrapperVariable Wc(std::forward<ARGS>(args)...);

    II_->add(Wc.name(), IdentifierIndex::CONST);

    Variables_.push_back(Wc);
  }

  template<typename ...ARGS>
  void addWrapperRecord(ARGS&&... args)
  {
    WrapperRecord Wr(std::forward<ARGS>(args)...);

    if (Wr.needsImplicitDefaultConstructor())
      addWrapperFunction(Wr.implicitDefaultConstructor());

    if (Wr.needsImplicitDestructor())
      addWrapperFunction(Wr.implicitDestructor());

    II_->add(Wr.name(), IdentifierIndex::TYPE);

    Records_.push_back(Wr);
  }

  template<typename ...ARGS>
  void addWrapperFunction(ARGS&&... args)
  {
    WrapperFunction Wf(std::forward<ARGS>(args)...);

    if (II_->has(Wf.name()))
      II_->pushOverload(Wf.name());
    else
      II_->add(Wf.name(), IdentifierIndex::FUNC);

    Functions_.push_back(Wf);
  }

  void overload()
  {
    for (auto &Wf : Functions_)
      Wf.overload(II_);
  }

  std::string wrappedFile() const
  { return WrappedFile_; }

  std::vector<WrapperVariable> constants() const
  { return Variables_; }

  std::vector<WrapperRecord> records() const
  { return Records_; }

  std::vector<WrapperFunction> functions() const
  { return Functions_; }

private:
  std::shared_ptr<IdentifierIndex> II_;

  std::string WrappedFile_;

  std::vector<WrapperVariable> Variables_;
  std::vector<WrapperRecord> Records_;
  std::vector<WrapperFunction> Functions_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_H
