from collections import OrderedDict

from cppbind import Record # XXX refactor

from text import code


class TypeInfo:
    NS = "__type_info"

    TYPED_PTR = f"{NS}::typed_ptr"

    OWN = f"{NS}::_own"
    DISOWN = f"{NS}::_disown"
    DELETE = f"{NS}::_delete"

    def __init__(self, wrapper):
        self._types = OrderedDict()

        def add_type(t, t_bases=None):
            if t_bases is not None:
                t_bases = tuple(t_bases)

            self._types[(t.without_const(), t_bases)] = True

        for r in Record.ordering(): # XXX refactor
            add_type(r.type, (b.type for b in r.bases_recursive))

        for v in wrapper.variables():
            if v.type.is_pointer() and not v.pointee().is_record():
                add_type(v.type.pointee())

        for f in wrapper.functions():
            for t in [f.return_type] + [p.type for p in f.parameters]:
                if t.is_pointer() and not t.pointee().is_record():
                    add_type(t.pointee())

    @classmethod
    def make_typed(cls, arg, owning=False):
        owning = 'true' if owning else 'false'

        return f"{cls.NS}::make_typed({arg}, {owning})"

    @classmethod
    def get_typed(cls, arg):
        return f"{cls.NS}::get_typed({arg})"

    @classmethod
    def typed_pointer_cast(cls, t, arg):
        return f"{cls.NS}::typed_pointer_cast<{t}>({arg})"

    @classmethod
    def own(cls, arg):
        return f"{cls.NS}::_own({arg})"

    @classmethod
    def disown(cls, arg):
        return f"{cls.NS}::_disown({arg})"

    @classmethod
    def delete(cls, arg):
        return f"{cls.NS}::_delete({arg})"

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
                auto it(_types.find(typeid(typename std::remove_const<T>::type)));
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
              typed_ptr(T *obj, bool owning = false)
              : _obj(static_cast<void const *>(obj)),
                _type(type::get<T>()),
                _const(std::is_const<T>::value),
                _owning(owning)
              {}

              ~typed_ptr()
              {
                if (_owning)
                  _type->destroy(_obj);
              }

              void own()
              { _owning = true; }

              void disown()
              { _owning = false; }

              template<typename T>
              typename std::enable_if<std::is_const<T>::value, T *>::type
              cast() const
              {
                if (_const)
                  return static_cast<T *>(_type->cast(type::get<T>(), _obj));

                return static_cast<T *>(_type->cast(type::get<T>(), const_cast<void *>(_obj)));
              }

              template<typename T>
              typename std::enable_if<!std::is_const<T>::value, T *>::type
              cast() const
              {
                if (_const)
                  throw std::bad_cast();

                return static_cast<T *>(_type->cast(type::get<T>(), const_cast<void *>(_obj)));
              }

            private:
              void const *_obj;
              type const *_type;
              bool _const;
              bool _owning;
            };

            template<typename T, typename ...ARGS>
            void *make_typed(T *obj, ARGS &&...args)
            {
              auto ptr = new typed_ptr(obj, std::forward<ARGS>(args)...);
              return static_cast<void *>(ptr);
            }

            typed_ptr *get_typed(void *ptr)
            { return static_cast<typed_ptr *>(ptr); }

            template<typename T>
            T *typed_pointer_cast(void *ptr)
            { return get_typed(ptr)->cast<T>(); }

            void *_own(void *obj)
            {
              get_typed(obj)->own();
              return obj;
            }

            void *_disown(void *obj)
            {
              get_typed(obj)->disown();
              return obj;
            }

            void _delete(void *obj)
            { delete get_typed(obj); }
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
