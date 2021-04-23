#include <cassert>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "Logging.hpp"
#include "TypeIndex.hpp"
#include "Wrapper.hpp"

namespace
{
  template<typename T>
  static std::vector<T const *> dequeToVector(std::deque<T> const &D) {
    std::vector<T const *> V;
    V.reserve(D.size());

    for (auto const &Obj : D)
      V.push_back(&Obj);

    return V;
  }
}

namespace cppbind
{

void
Wrapper::overload()
{
  for (auto &Wr : Records_)
    Wr.overload(II_);

  for (auto &Wf : Functions_)
    Wf.overload(II_);
}

std::vector<WrapperDefinition const *>
Wrapper::getDefinitions() const
{ return dequeToVector(Definitions_); }

std::vector<WrapperEnum const *>
Wrapper::getEnums() const
{ return dequeToVector(Enums_); }

std::vector<WrapperConstant const *>
Wrapper::getConstants() const
{ return dequeToVector(Constants_); }

std::vector<WrapperFunction const *>
Wrapper::getFunctions() const
{
  std::vector<WrapperFunction const *> Funcs;
  Funcs.reserve(Functions_.size());

  for (auto const &Func : Functions_)
    Funcs.push_back(&Func);

  return Funcs;
}

std::vector<WrapperRecord const *>
Wrapper::getRecords() const
{
  std::vector<WrapperRecord const *> Records;

  for (auto Type : TI_->basesFirstOrdering()) {
    auto It(RecordTypesMangled_.find(Type));
    if (It == RecordTypesMangled_.end())
      continue;

    auto const *Record(It->second);

    Records.push_back(Record);
  }

  return Records;
}

bool
Wrapper::_addWrapperEnum(WrapperEnum *Enum)
{
  for (auto const &Constant : Enum->getConstants())
    II_->addDefinition(Constant.getName(), IdentifierIndex::CONST);

  return true;
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

  auto &Params(Function->getParameters());

  for (std::size_t i = 0; i < Params.size(); ++i) {
    if (checkTypeWrapped(Params[i].getType()))
      continue;

    if (Params[i].getDefaultArgument()) {
      log::debug("on second thought, pruning parameters instead");

      for (std::size_t j = 0; j < Params.size() - i; ++j)
        Params.pop_back();

      break;

    } else {
      return false;
    }
  }

  auto FunctionNameTemplated(Function->getName(true, true));

  auto It(FunctionNames_.find(FunctionNameTemplated));
  if (It == FunctionNames_.end()) {
    FunctionNames_[FunctionNameTemplated].push_back(Function);

    if (Function->isDefinition())
      II_->addDefinition(FunctionNameTemplated, IdentifierIndex::FUNC);
    else
      II_->addDeclaration(FunctionNameTemplated, IdentifierIndex::FUNC);

    return true;
  }
  for (auto &Prev : It->second) {
    if (*Prev == *Function) {
      assert(!Prev->isDefinition() && Function->isDefinition());

      II_->addDefinition(FunctionNameTemplated, IdentifierIndex::FUNC);

      return false;
    }
  }

  FunctionNames_[FunctionNameTemplated].push_back(Function);

  if (Function->isDefinition())
    II_->addDefinition(FunctionNameTemplated, IdentifierIndex::FUNC);
  else
    II_->addDeclaration(FunctionNameTemplated, IdentifierIndex::FUNC);

  II_->pushOverload(FunctionNameTemplated);

  return true;
}

bool
Wrapper::_addWrapperRecord(WrapperRecord *Record)
{
  auto RecordNameTemplated(Record->getName(true));
  auto RecordTypeMangled(Record->getType().mangled());

  if (!Record->isDefinition()) {
    log::debug("not a definition");

    II_->addDeclaration(RecordNameTemplated, IdentifierIndex::RECORD);
    TI_->addProto(RecordTypeMangled);

    return false;
  }

  std::deque<std::string> BaseTypesMangled;
  for (auto const &BaseType : Record->getType().baseTypes()) {
    auto BaseTypeMangled(BaseType.mangled());
    BaseTypesMangled.push_back(BaseTypeMangled);

    auto It(RecordTypesMangled_.find(BaseTypeMangled));
    if (It != RecordTypesMangled_.end())
      Record->getBases().push_back(It->second);
  }

  II_->addDefinition(RecordNameTemplated, IdentifierIndex::RECORD);
  TI_->add(RecordTypeMangled, BaseTypesMangled.begin(), BaseTypesMangled.end());
  RecordTypesMangled_[RecordTypeMangled] = Record;

  auto &Functions(Record->getFunctions());

  auto It(Functions.begin());
  while (It != Functions.end()) {
    log::debug("considering member {0}", *It);

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
  else if (Type.isRecordIndirection(true))
    RecordType = Type.pointee(true);
  else
    return true;

  RecordType = RecordType.withoutConst();

  return TI_->hasProto(RecordType.mangled());
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
