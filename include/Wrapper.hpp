#ifndef GUARD_WRAPPER_H
#define GUARD_WRAPPER_H

#include <deque>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/labeled_graph.hpp"

#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "WrapperVariable.hpp"
#include "WrapperFunction.hpp"
#include "WrapperRecord.hpp"
#include "WrapperType.hpp"

namespace cppbind
{

class Wrapper
{
  using FunctionNameLookup = std::unordered_map<Identifier,
                                                WrapperFunction const *>;

  using RecordTypeLookup = std::unordered_map<WrapperType,
                                              WrapperRecord const *> ;

  class RecordInheritanceGraph
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

  using RecordWithBases = std::pair<WrapperRecord const *,
                                    std::vector<WrapperRecord const *>>;

public:
  template<typename ...ARGS>
  void addWrapperVariable(std::shared_ptr<IdentifierIndex> II, ARGS &&...Args)
  {
    Variables_.emplace_back(std::forward<ARGS>(Args)...);
    addVariable(II, &Variables_.back());
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

  std::vector<WrapperVariable const *> getVariables() const;
  std::vector<WrapperFunction const *> getFunctions() const;
  std::vector<RecordWithBases> getRecords() const;

private:
  void addVariable(std::shared_ptr<IdentifierIndex> II,
                   WrapperVariable const *Variable);

  void addFunction(std::shared_ptr<IdentifierIndex> II,
                   WrapperFunction const *Function);

  void addRecord(std::shared_ptr<IdentifierIndex> II,
                 WrapperRecord const *Record);

  WrapperFunction const *getFunctionFromName(Identifier const &Name) const;

  WrapperRecord const *getRecordFromType(WrapperType const &Type) const;

  std::deque<WrapperVariable> Variables_;

  std::deque<WrapperFunction> Functions_;
  FunctionNameLookup FunctionNames_;

  std::deque<WrapperRecord> Records_;
  RecordTypeLookup RecordTypes_;
  RecordInheritanceGraph RecordInheritances_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_H
