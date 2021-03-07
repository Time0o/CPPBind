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

    if (!addWrapperConstant(II, &Constants_.back()))
      Constants_.pop_back();
    else
      log::info("created {0}", Constants_.back());
  }

  template<typename ...ARGS>
  void addWrapperFunction(std::shared_ptr<IdentifierIndex> II, ARGS &&...Args)
  {
    Functions_.emplace_back(std::forward<ARGS>(Args)...);

    if (!addWrapperFunction(II, &Functions_.back()))
      Functions_.pop_back();
    else
      log::info("created {0}", Functions_.back());
  }

  template<typename ...ARGS>
  void addWrapperRecord(std::shared_ptr<IdentifierIndex> II, ARGS &&...Args)
  {
    Records_.emplace_back(std::forward<ARGS>(Args)...);

    if (!addWrapperRecord(II, &Records_.back()))
      Records_.pop_back();
    else
      log::info("created {0}", Records_.back());
  }

  void overload(std::shared_ptr<IdentifierIndex> II);

  std::vector<WrapperConstant const *> getConstants() const;

  std::vector<WrapperFunction const *> getFunctions() const;

  std::vector<std::pair<WrapperRecord const *,
              std::vector<WrapperRecord const *>>> getRecords() const;

private:
  bool addWrapperConstant(std::shared_ptr<IdentifierIndex> II,
                          WrapperConstant *Constant);

  bool addWrapperFunction(std::shared_ptr<IdentifierIndex> II,
                          WrapperFunction *Function);

  bool addWrapperRecord(std::shared_ptr<IdentifierIndex> II,
                        WrapperRecord *Record);

  template<typename T>
  bool checkTypeWrapped(std::shared_ptr<IdentifierIndex> II,
                        WrapperType const &Type,
                        T const *Obj) const
  {
    if (typeWrapped(II, Type))
      return true;

    if (OPT(bool, "skip-unwrappable")) {
      log::debug("{0}: skipping because type '{1}' is unwrapped", *Obj, Type);
      return false;
    } else {
      throw log::exception("{0}: type '{1}' is unwrapped", *Obj, Type);
    }
  }

  bool typeWrapped(std::shared_ptr<IdentifierIndex> II,
                   WrapperType const &Type) const;

  std::deque<WrapperConstant> Constants_;

  std::deque<WrapperFunction> Functions_;

  std::deque<WrapperRecord> Records_;
  std::unordered_map<WrapperType, WrapperRecord const *> RecordTypes_;
  InheritanceGraph RecordInheritances_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_H
