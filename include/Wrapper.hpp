#ifndef GUARD_WRAPPER_H
#define GUARD_WRAPPER_H

#include <deque>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/labeled_graph.hpp"

#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "Logging.hpp"
#include "Options.hpp"
#include "WrapperConstant.hpp"
#include "WrapperFunction.hpp"
#include "WrapperRecord.hpp"
#include "WrapperType.hpp"

namespace cppbind
{

class Wrapper
{
  template<typename T, typename T_KEY>
  using Lookup = std::unordered_map<T_KEY, T const *>;

  template<typename T>
  using NameLookup = Lookup<T, Identifier>;

  template<typename T>
  using TypeLookup = Lookup<T, WrapperType>;

  class InheritanceGraph
  {
  public:
    void add(WrapperType const &Type, std::vector<WrapperType> const &BaseTypes);

    std::deque<WrapperType>
    bases(WrapperType const &Type, bool Recursive = false) const;

    std::deque<WrapperType>
    basesFirstOrdering() const;

  private:
    using Graph = boost::adjacency_list<boost::vecS,
                                        boost::vecS,
                                        boost::directedS,
                                        WrapperType>;

    using LabeledGraph = boost::labeled_graph<Graph,
                                              WrapperType,
                                              boost::hash_mapS>;

    LabeledGraph G_;
  };

public:
  template<typename ...ARGS>
  void addWrapperConstant(std::shared_ptr<IdentifierIndex> II, ARGS &&...Args)
  {
    Constants_.emplace_back(std::forward<ARGS>(Args)...);
    addConstant(II, &Constants_.back());
  }

  template<typename ...ARGS>
  void addWrapperFunction(std::shared_ptr<IdentifierIndex> II, ARGS &&...Args)
  {
    Functions_.emplace_back(std::forward<ARGS>(Args)...);
    addFunction(II, &Functions_.back());
  }

  template<typename ...ARGS>
  void addWrapperRecord(std::shared_ptr<IdentifierIndex> II, ARGS &&...Args)
  {
    Records_.emplace_back(std::forward<ARGS>(Args)...);
    addRecord(II, &Records_.back());
  }

  void overload(std::shared_ptr<IdentifierIndex> II);

  std::vector<WrapperConstant const *> getConstants() const;

  std::vector<WrapperFunction const *> getFunctions() const;

  std::vector<std::pair<WrapperRecord const *,
              std::vector<WrapperRecord const *>>> getRecords() const;

private:
  void addConstant(std::shared_ptr<IdentifierIndex> II,
                   WrapperConstant const *Constant);

  void addFunction(std::shared_ptr<IdentifierIndex> II,
                   WrapperFunction const *Function);

  void addRecord(std::shared_ptr<IdentifierIndex> II,
                 WrapperRecord const *Record);

  template<typename T, typename T_KEY>
  T const *objLookup(Lookup<T, T_KEY> const &Lookup, T_KEY const &Key) const
  {
    auto It(Lookup.find(Key));

    if (It == Lookup.end())
      return nullptr;

    return It->second;
  }

  template<typename T>
  bool objCheckTypeWrapped(T const *Obj, WrapperType const &Type)
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

    if (objLookup(RecordTypes_, RecordType.withoutConst()))
      return true;

    if (OPT(bool, "skip-unwrappable")) {
      warning("{0}: skipping because type '{1}' is unwrapped", *Obj, Type);
      return false;
    } else {
      throw exception("{0}: type '{1}' is unwrapped", *Obj, Type);
    }
  }

  std::deque<WrapperConstant> Constants_;
  NameLookup<WrapperConstant> ConstantNames_;
  TypeLookup<WrapperConstant> ConstantTypes_;

  std::deque<WrapperFunction> Functions_;
  NameLookup<WrapperFunction> FunctionNames_;

  std::deque<WrapperRecord> Records_;
  NameLookup<WrapperRecord> RecordNames_;
  TypeLookup<WrapperRecord> RecordTypes_;
  InheritanceGraph RecordInheritances_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_H
