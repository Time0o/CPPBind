from collections import OrderedDict

from cppbind import Record # XXX refactor

from text import code


class TypeInfo:
    MAKE_TYPED = "__type_info::make_typed"
    TYPED_POINTER_CAST = "__type_info::typed_pointer_cast"

    def __init__(self, wrapper):
        self._types = OrderedDict()

        def add_type_instance(t, t_bases=None):
            if t.is_record():
                self._types[(t, t_bases)] = True
            elif t.is_pointer():
                self._types[(t.pointee(), t_bases)] = True

        for r in Record.ordering(): # XXX refactor
            add_type_instance(r.type, tuple(b.type for b in r.bases_recursive))

        for v in wrapper.variables():
            add_type_instance(v.type)

        for f in wrapper.functions():
            add_type_instance(f.return_type)

            for p in f.parameters:
                add_type_instance(p.type)

    def code(self):
        if not self._types:
            return

        return code(
            """
            #include <cassert>
            #include <typeindex>
            #include <typeinfo>
            #include <unordered_map>

            namespace __type_info
            {{
              {type_utils}

              {type_instances}
            }}
            """,
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
              template<typename B>
              static void const *cast(void const *obj)
              {
                auto const *base = static_cast<B const*>(static_cast<T const *>(obj));

                return static_cast<void const *>(base);
              }

              void add_type() const
              { _types.insert(std::make_pair(std::type_index(typeid(T)), this)); }

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
              typed_ptr(type const *type, void const *obj)
              : _type(type),
                _obj(obj)
              {}

              void const *cast(type const *to) const
              { return _type->cast(to, _obj); }

              void *cast(type const *to)
              { return const_cast<void *>(const_cast<typed_ptr const *>(this)->cast(to)); }

            private:
              type const *_type;
              void const *_obj;
            };

            template<typename T>
            void const *make_typed(T const *obj)
            {
              auto const *ptr = new typed_ptr(type::get<T>(), static_cast<void const *>(obj));

              return static_cast<void const *>(ptr);
            }

            template<typename T>
            void *make_typed(T *obj)
            { return const_cast<void *>(make_typed(const_cast<T const *>(obj))); }

            template<typename T>
            T const *typed_pointer_cast(void const *ptr)
            {
              void const *obj = static_cast<typed_ptr const *>(ptr)->cast(type::get<T>());

              return static_cast<T const *>(obj);
            }

            template<typename T>
            T *typed_pointer_cast(void *ptr)
            { return const_cast<T *>(typed_pointer_cast<T>(const_cast<void const *>(ptr))); }
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
