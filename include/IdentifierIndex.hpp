#ifndef GUARD_IDENTIFIER_INDEX_H
#define GUARD_IDENTIFIER_INDEX_H

#include <cassert>
#include <map>
#include <memory>

#include "Identifier.hpp"

namespace cppbind
{

class IdentifierIndex
{
public:
  enum Type
  {
    ANY,
    CONST,
    TYPE,
    FUNC
  };

private:
  struct Props
  {
    explicit Props(Type Type) : Type(Type) {}

    Type Type;
  };

  struct AnyProps : public Props
  { AnyProps() : Props(ANY) {} };

  struct ConstProps : public Props
  { ConstProps() : Props(CONST) {} };

  struct TypeProps : public Props
  { TypeProps() : Props(TYPE) {} };

  struct FuncProps : public Props
  {
    FuncProps() : Props(FUNC) {}

    unsigned CurrentOverload = 1u;
    unsigned MaxOverload = 1u;
  };

public:
  void add(Identifier const &Id, Type Type)
  {
    assert(!has(Id));

    std::shared_ptr<Props> P;

    switch (Type) {
      case ANY:
        P = std::make_shared<AnyProps>();
        break;
      case CONST:
        P = std::make_shared<ConstProps>();
        break;
      case TYPE:
        P = std::make_shared<TypeProps>();
        break;
      case FUNC:
        P = std::make_shared<FuncProps>();
        break;
    }

    Index_[Id] = P;
  }

  bool has(Identifier const &Id) const
  { return static_cast<bool>(props<AnyProps>(Id)); }

  bool has(Identifier const &Id, Type Type) const
  {
    auto Props(props<AnyProps>(Id));
    if (!Props)
      return false;

    return Props->Type == Type;
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

    if (P->MaxOverload == 1u)
      return 0u;

    assert(P->CurrentOverload <= P->MaxOverload);

    return P->CurrentOverload++;
  }

private:
  template<typename T>
  std::shared_ptr<T> props(Identifier const &Id) const
  {
    auto It(Index_.find(Id));

    if (It == Index_.end())
      return nullptr;

    auto Props(It->second);

#ifndef NDEBUG
    if constexpr (std::is_same_v<T, ConstProps>)
      assert(Props->Type = CONST);
    else if constexpr (std::is_same_v<T, TypeProps>)
      assert(Props->Type = TYPE);
    else if constexpr (std::is_same_v<T, FuncProps>)
      assert(Props->Type = FUNC);
#endif

    return std::static_pointer_cast<T>(Props);
  }

  std::map<Identifier, std::shared_ptr<Props>> Index_;
};

} // namespace cppbind

#endif // GUARD_IDENTIFIER_INDEX_H
