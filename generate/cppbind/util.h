#ifndef GUARD_CPPBIND_UTIL_H
#define GUARD_CPPBIND_UTIL_H

#include <cstdint>

namespace cppbind
{

namespace util
{

inline std::size_t string_hash(char const *str)
{
  std::size_t h = 5381;

  int c = 0;
  while ((c = *str++))
    h = ((h << 5) + h) + c;

  return h;
}

template<typename T_KEY, typename T_VAL>
class simple_map
{
  enum : std::size_t { TABLE_SIZE = 100 };

  struct node
  {
    node(T_KEY key, T_VAL val)
    : key(key),
      val(val),
      next(nullptr)
    {}

    T_KEY key;
    T_VAL val;

    node *next;
  };

public:
  simple_map()
  : table()
  {}

  ~simple_map()
  {
    for (std::size_t offs = 0; offs < TABLE_SIZE; ++offs) {
      node *head = table[offs];
      while (head) {
        node *next = head->next;
        delete head;
        head = next;
      }
    }
  }

  void insert(T_KEY const &key, T_VAL const &val)
  {
    std::size_t offs = table_offs(key);

    node *next = table[offs];
    node *head = new node(key, val);
    head->next = next;
    table[offs] = head;
  }

  T_VAL const *find(T_KEY const &key) const
  {
    std::size_t offs = table_offs(key);

    node *head = table[offs];
    while (head) {
      if (head->key == key)
        return &head->val;
    }

    return nullptr;
  }

private:
  static std::size_t table_offs(T_KEY const &key)
  { return reinterpret_cast<std::size_t>(key) % TABLE_SIZE; }

  node *table[TABLE_SIZE];
};

} // namespace util

} // namespace cppbind

#endif // GUARD_CPPBIND_UTIL_H
