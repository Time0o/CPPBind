#include <cassert>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

#include "CompilerState.hpp"
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
    Wr.overload(CompilerState().identifiers());

  for (auto &Wf : Functions_)
    Wf.overload(CompilerState().identifiers());
}

std::vector<WrapperInclude const *>
Wrapper::getIncludes() const
{ return dequeToVector(Includes_); }

std::vector<WrapperMacro const *>
Wrapper::getMacros() const
{ return dequeToVector(Macros_); }

std::vector<WrapperEnum const *>
Wrapper::getEnums() const
{ return dequeToVector(Enums_); }

std::vector<WrapperVariable const *>
Wrapper::getVariables() const
{ return dequeToVector(Variables_); }

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

  return CompilerState().types()->getRecordBasesFirstOrdering();
}

bool
Wrapper::_addWrapperEnum(WrapperEnum *Enum)
{
  CompilerState().types()->addEnumDefinition(Enum);

  return true;
}

bool
Wrapper::_addWrapperVariable(WrapperVariable *Variable)
{
  if (!checkTypeWrapped(Variable->getType()))
    return false;

  CompilerState().identifiers()->addDefinition(Variable->getName(),
                                               IdentifierIndex::CONST);

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

  auto FunctionNameTemplated(Function->getFormat(true, false, true));

  auto It(FunctionNames_.find(FunctionNameTemplated));
  if (It == FunctionNames_.end()) {
    FunctionNames_[FunctionNameTemplated].push_back(Function);

    if (Function->isDefinition()) {
      CompilerState().identifiers()->addDefinition(FunctionNameTemplated,
                                                   IdentifierIndex::FUNC);
    } else {
      CompilerState().identifiers()->addDeclaration(FunctionNameTemplated,
                                                    IdentifierIndex::FUNC);
    }

    return true;
  }
  for (auto &Prev : It->second) {
    if (*Prev == *Function) {
      assert(!Prev->isDefinition() && Function->isDefinition());

      CompilerState().identifiers()->addDefinition(FunctionNameTemplated,
                                                   IdentifierIndex::FUNC);

      return false;
    }
  }

  FunctionNames_[FunctionNameTemplated].push_back(Function);

  if (Function->isDefinition()) {
    CompilerState().identifiers()->addDefinition(FunctionNameTemplated,
                                                 IdentifierIndex::FUNC);
  } else {
    CompilerState().identifiers()->addDeclaration(FunctionNameTemplated,
                                                  IdentifierIndex::FUNC);
  }

  CompilerState().identifiers()->pushOverload(FunctionNameTemplated);

  return true;
}

bool
Wrapper::_addWrapperRecord(WrapperRecord *Record)
{
  auto RecordNameTemplated(Record->getFormat(true));
  auto RecordTypeMangled(Record->getType().mangled());

  if (Record->isDefinition()) {
    CompilerState().identifiers()->addDefinition(RecordNameTemplated,
                                                 IdentifierIndex::RECORD);

    CompilerState().types()->addRecordDefinition(Record);

    auto &Functions(Record->getFunctions());

    auto It(Functions.begin());
    while (It != Functions.end()) {
      log::debug("considering member {0}", *It);

      if (!_addWrapperFunction(&(*It)))
        It = Functions.erase(It);
      else
        ++It;
    }

  } else {
    log::debug("not a definition");

    CompilerState().identifiers()->addDeclaration(RecordNameTemplated,
                                                  IdentifierIndex::RECORD);

    CompilerState().types()->addRecordDeclaration(Record);
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

  return CompilerState().types()->hasRecordDeclaration(RecordType);
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
