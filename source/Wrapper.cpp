#include <cassert>
#include <deque>
#include <memory>
#include <vector>

#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "Logging.hpp"
#include "TypeIndex.hpp"
#include "Wrapper.hpp"

namespace cppbind
{

void
Wrapper::finalize()
{
  for (auto &Wr : Records_)
    Wr.overload(II_);

  for (auto &Wf : Functions_)
    Wf.overload(II_);
}

std::vector<WrapperConstant const *>
Wrapper::getConstants() const
{
  std::vector<WrapperConstant const *> Consts;
  Consts.reserve(Constants_.size());

  for (auto const &Const : Constants_)
    Consts.push_back(&Const);

  return Consts;
}

std::vector<WrapperFunction const *>
Wrapper::getFunctions() const
{
  std::vector<WrapperFunction const *> Funcs;
  Funcs.reserve(Functions_.size());

  for (auto const &Func : Functions_)
    Funcs.push_back(&Func);

  return Funcs;
}

std::vector<std::pair<WrapperRecord const *,
            std::vector<WrapperRecord const *>>>
Wrapper::getRecords() const
{
  std::vector<std::pair<WrapperRecord const *,
              std::vector<WrapperRecord const *>>> RecordsAndBases;

  auto recordFromType = [&](WrapperType const &Type)
  {
    for (auto const &Record : Records_) {
      if (Record.getType() == Type)
        return &Record;
    }

    assert(false);
  };

  for (auto Type : TI_->basesFirstOrdering()) {
    auto const *Record(recordFromType(Type));

    std::vector<WrapperRecord const *> Bases;
    for (auto const &BaseType : TI_->bases(Type, true))
      Bases.push_back(recordFromType(BaseType));

    RecordsAndBases.push_back(std::make_pair(Record, Bases));
  }

  return RecordsAndBases;
}

bool
Wrapper::_addWrapperConstant(WrapperConstant *Constant)
{
  if (!checkTypeWrapped(Constant->getType()))
    return false;

  II_->addDefinition(Constant->getName(), IdentifierIndex::CONST);

  return true;
}

bool
Wrapper::_addWrapperFunction(WrapperFunction *Function)
{
  if (!checkTypeWrapped(Function->getReturnType()))
    return false;

  for (auto const &Param : Function->getParameters()) {
    if (!checkTypeWrapped(Param.getType()))
      return false;
  }

  auto FunctionNameTemplated(Function->getName(true));

  if (II_->hasDefinition(FunctionNameTemplated, IdentifierIndex::FUNC)) {
    II_->pushOverload(FunctionNameTemplated);

  } else if (II_->hasDeclaration(FunctionNameTemplated, IdentifierIndex::FUNC)) {
    if (Function->isDefinition()) {
      II_->addDefinition(FunctionNameTemplated, IdentifierIndex::FUNC);
      return false;
    }

    II_->pushOverload(FunctionNameTemplated);

  } else {
    if (Function->isDefinition())
      II_->addDefinition(FunctionNameTemplated, IdentifierIndex::FUNC);
    else
      II_->addDeclaration(FunctionNameTemplated, IdentifierIndex::FUNC);
  }

  return true;
}

bool
Wrapper::_addWrapperRecord(WrapperRecord *Record)
{
  auto RecordNameTemplated(Record->getName(true));

  if (!Record->isDefinition()) {
    II_->addDeclaration(RecordNameTemplated, IdentifierIndex::RECORD);
    return false;
  }

  II_->addDefinition(RecordNameTemplated, IdentifierIndex::RECORD);

  TI_->add(Record->getType(),
           Record->getBaseTypes().begin(),
           Record->getBaseTypes().end());

  auto &Functions(Record->getFunctions());

  auto It(Functions.begin());
  while (It != Functions.end()) {
    if (!_addWrapperFunction(&(*It)))
      It = Functions.erase(It);
    else
      ++It;
  }

  return true;
}

bool
Wrapper::typeWrapped(WrapperType const &Type) const
{
  WrapperType RecordType;

  if (Type.isRecord())
    RecordType = Type;
  else if (Type.isPointer() && Type.pointee().isRecord())
    RecordType = Type.pointee();
  else if (Type.isReference() && Type.referenced().isRecord())
    RecordType = Type.referenced();
  else
    return true;

  RecordType = RecordType.withoutConst();

  return TI_->has(RecordType);
}

bool
Wrapper::checkTypeWrapped(WrapperType const &Type) const
{
  if (typeWrapped(Type))
    return true;

  if (OPT(bool, "wrap-skip-unwrappable")) {
    log::debug("skipping because type '{0}' is unwrapped", Type);
    return false;
  } else {
    throw log::exception("type '{0}' is unwrapped", Type);
  }
}

} // namespace cppbind
