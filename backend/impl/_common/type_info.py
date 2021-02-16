from collections import OrderedDict

from cppbind import Record # XXX refactor

from text import code


class TypeInfo:
    NS = "__type_info"
    TYPED_PTR = f"{NS}::typed_ptr"

    def __init__(self, wrapper):
        self._types = OrderedDict()

        for r in Record.ordering(): # XXX refactor
            self._types[(r.type, tuple(b.type for b in r.bases_recursive))] = True

        for v in wrapper.variables():
            if v.type.is_pointer() and not v.pointee().is_record():
                self._types[(v.type.pointee(), None)] = True

        for f in wrapper.functions():
            for t in [f.return_type] + [p.type for p in f.parameters]:
                if t.is_pointer() and not t.pointee().is_record():
                    self._types[(t.pointee(), None)] = True

    @classmethod
    def make_typed(cls, arg, owned=False):
        owned = 'true' if owned else 'false'

        return f"{cls.NS}::make_typed({arg}, {owned})"

    @classmethod
    def get_typed(cls, arg):
        return f"{cls.NS}::get_typed({arg})"

    @classmethod
    def typed_pointer_cast(cls, t, arg):
        return f"{cls.NS}::typed_pointer_cast<{t}>({arg})"

    def code(self):
        return code(
            """
            #include <cassert>
            #include <typeindex>
            #include <typeinfo>
            #include <unordered_map>
            #include <utility>

            namespace {ns}
            {{

            {type_utils}

            {type_instances}

            }}
            """,
            ns=self.NS,
            type_utils=self._type_utils(),
            type_instances=self._type_instances())

    @staticmethod
    def _type_utils():
        return code(
            """
            class type
            {
            public:
              template<typename T>
              static type const *get()
              {
                auto it(_types.find(typeid(T)));
                assert(it != _types.end());

                return it->second;
              }

              virtual void destroy(void const *obj) const = 0;

              virtual void const *cast(type const *to, void const *obj) const = 0;
              virtual void *cast(type const *to, void *obj) const = 0;

            protected:
              static std::unordered_map<std::type_index, type const *> _types;
            };

            std::unordered_map<std::type_index, type const *> type::_types;

            template<typename T, typename ...BS>
            class type_instance : public type
            {
            public:
              type_instance()
              {
                add_type();
                add_cast<T>();
                add_base_casts();
              }

              void destroy(void const *obj) const override
              { delete static_cast<T const *>(obj); }

              void const *cast(type const *to, void const *obj) const override
              {
                auto it(_casts.find(to));
                if (it == _casts.end())
                    throw std::bad_cast();

                return it->second(obj);
              }

              void *cast(type const *to, void *obj) const override
              { return const_cast<void *>(cast(to, const_cast<void const *>(obj))); }

            private:
              void add_type() const
              { _types.insert(std::make_pair(std::type_index(typeid(T)), this)); }

              template<typename U>
              static void const *cast(void const *obj)
              {
                auto const *from = static_cast<T const *>(obj);
                auto const *to = static_cast<U const *>(from);

                return static_cast<void const *>(to);
              }

              template<typename B>
              void add_cast()
              { _casts.insert(std::make_pair(type::get<B>(), cast<B>)); }

              void add_base_casts()
              { [](...){}((add_cast<BS>(), 0)...); }

              std::unordered_map<type const *, void const *(*)(void const *)> _casts;
            };

            class typed_ptr
            {
            public:
              template<typename T>
              typed_ptr(T const *obj, bool owning = false)
              : _type(type::get<T>()),
                _obj(static_cast<void const *>(obj)),
                _owning(owning)
              {}

              ~typed_ptr()
              {
                if (_owning)
                  _type->destroy(_obj);
              }

              void const *cast(type const *to) const
              { return _type->cast(to, _obj); }

              void *cast(type const *to)
              { return const_cast<void *>(const_cast<typed_ptr const *>(this)->cast(to)); }

            private:
              type const *_type;
              void const *_obj;
              bool _owning;
            };

            template<typename T, typename ...ARGS>
            void const *make_typed(T const *obj, ARGS &&...args)
            {
              auto ptr = new typed_ptr(obj, std::forward<ARGS>(args)...);
              return static_cast<void const *>(ptr);
            }

            template<typename T, typename ...ARGS>
            void *make_typed(T *obj, ARGS &&...args)
            {
              auto ptr = make_typed(const_cast<T const *>(obj),
                                    std::forward<ARGS>(args)...);
              return const_cast<void *>(ptr);
            }

            template<typename T>
            typed_ptr *get_typed(T const *obj)
            {
              auto ptr = static_cast<typed_ptr const *>(obj);
              return const_cast<typed_ptr *>(ptr);
            }

            template<typename T>
            typed_ptr *get_typed(T *obj)
            {
              auto ptr = get_typed(const_cast<T const *>(obj));
              return const_cast<typed_ptr *>(ptr);
            }

            template<typename T>
            T const *typed_pointer_cast(void const *ptr)
            {
              auto obj = static_cast<typed_ptr const *>(ptr)->cast(type::get<T>());
              return static_cast<T const *>(obj);
            }

            template<typename T>
            T *typed_pointer_cast(void *ptr)
            {
              auto obj = typed_pointer_cast<T>(const_cast<void const *>(ptr));
              return const_cast<T *>(obj);
            }
            """)

    def _type_instances(self):
        tis = []
        for t, t_bases in self._types.keys():
            template_params = [t]

            if t_bases is not None:
                template_params += t_bases

            template_params = ', '.join(map(str, template_params))

            tis.append(f"type_instance<{template_params}> {t.format(mangled=True)};")

        return '\n'.join(tis)
