#ifndef GUARD_IDENTIFIER_INDEX_H
#define GUARD_IDENTIFIER_INDEX_H

#include <cassert>
#include <map>
#include <memory>
#include <utility>

#include "Identifier.hpp"

namespace cppbind
{

class IdentifierIndex
{
public:
  enum Type
  {
    TYPE,
    CONST,
    FUNC,
    RECORD
  };

private:
  struct Props
  {
    explicit Props(Type Type, bool IsDefinition)
    : Type(Type),
      IsDefinition(IsDefinition)
    {}

    Type Type;
    bool IsDefinition;
  };

  struct TypeProps : public Props
  {
    TypeProps()
    : Props(TYPE, true)
    {}
  };

  struct ConstProps : public Props
  {
    ConstProps()
    : Props(CONST, true)
    {}
  };

  struct FuncProps : public Props
  {
    FuncProps(bool IsDefinition)
    : Props(FUNC, IsDefinition)
    {}

    unsigned CurrentOverload = 1u;
    unsigned MaxOverload = 1u;
  };

  struct RecordProps : public Props
  {
    RecordProps(bool IsDefinition)
    : Props(RECORD, IsDefinition)
    {}
  };

public:
  Identifier addDeclaration(Identifier Id, Type Type)
  { return add(Id, Type, false); }

  Identifier addDefinition(Identifier Id, Type Type)
  { return add(Id, Type, true); }

  bool hasDeclaration(Identifier const &Id, Type Type) const
  { return has(Id, Type, false); }

  bool hasDefinition(Identifier const &Id, Type Type) const
  { return has(Id, Type, true); }

  bool hasOverload(Identifier const &Id) const
  {
    auto P(props<FuncProps>(Id));
    assert(P);

    return P->MaxOverload > 1u;
  }

  void pushOverload(Identifier const &Id)
  {
    auto P(props<FuncProps>(Id));
    assert(P);

    ++P->MaxOverload;
  }

  unsigned popOverload(Identifier const &Id) const
  {
    auto P(props<FuncProps>(Id));
    assert(P);
    assert(P->MaxOverload > 1u);

    assert(P->CurrentOverload <= P->MaxOverload);

    return P->CurrentOverload++;
  }

private:
  Identifier add(Identifier Id, Type Type, bool Definition)
  {
    // XXX conflict resolution

    std::shared_ptr<Props> P;

    switch (Type) {
      case TYPE:
        P = std::make_shared<TypeProps>();
        break;
      case CONST:
        P = std::make_shared<ConstProps>();
        break;
      case FUNC:
        P = std::make_shared<FuncProps>(Definition);
        break;
      case RECORD:
        P = std::make_shared<RecordProps>(Definition);
        break;
    }

    Index_[Id] = P;

    return Id;
  }

  bool has(Identifier const &Id, Type Type, bool Definition) const
  {
    auto Props(props<Props>(Id));
    if (!Props)
      return false;

    return Props->Type == Type && (!Definition || Props->IsDefinition);
  }

  template<typename T>
  std::shared_ptr<T> props(Identifier const &Id) const
  {
    auto It(Index_.find(Id));

    if (It == Index_.end())
      return nullptr;

    auto Props(It->second);

    return std::static_pointer_cast<T>(Props);
  }

  std::map<Identifier, std::shared_ptr<Props>> Index_;
};

} // namespace cppbind

#endif // GUARD_IDENTIFIER_INDEX_H
