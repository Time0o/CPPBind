from backend import Backend
from collections import OrderedDict
from cppbind import Record # XXX refactor
from text import code


NS = "__type_info"
TYPED_PTR = f"{NS}::typed_ptr"
OWN = f"{NS}::_own"
DISOWN = f"{NS}::_disown"
COPY = f"{NS}::_copy"
MOVE = f"{NS}::_move"
DELETE = f"{NS}::_delete"


def make_typed(arg, owning=False):
    owning = 'true' if owning else 'false'

    return f"{NS}::make_typed({arg}, {owning})"


def get_typed(arg):
    return f"{NS}::get_typed({arg})"


def typed_pointer_cast(t, arg):
    return f"{NS}::typed_pointer_cast<{t}>({arg})"


def own(arg):
    return f"{OWN}({arg})"


def disown(arg):
    return f"{DISOWN}({arg})"


def copy(arg):
    return f"{COPY}({arg})"


def move(arg):
    return f"{MOVE}({arg})"


def delete(arg):
    return f"{NS}::_delete({arg})"


def define():
    return code(
        """
        #include <cassert>
        #include <stdexcept>
        #include <type_traits>
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
        ns=NS,
        type_utils=_type_utils(),
        type_instances=_type_instances())


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

          virtual void *copy(void const *obj) const = 0;
          virtual void *move(void *obj) const = 0;
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

          void *copy(void const *obj) const override
          { return _copy(obj); }

          void *move(void *obj) const override
          { return _move(obj); }

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
          template<typename U = T>
          typename std::enable_if<std::is_copy_constructible<U>::value, void *>::type
          _copy(void const *obj) const
          {
            auto obj_copy = new T(*static_cast<T const *>(obj));
            return static_cast<void *>(obj_copy);
          }

          template<typename U = T>
          typename std::enable_if<!std::is_copy_constructible<U>::value, void *>::type
          _copy(void const *obj) const
          { throw std::runtime_error("not copy constructible"); }

          template<typename U = T>
          typename std::enable_if<std::is_move_constructible<U>::value, void *>::type
          _move(void *obj) const
          {
            auto obj_moved = new T(std::move(*static_cast<T *>(obj)));
            return static_cast<void *>(obj_moved);
          }

          template<typename U = T>
          typename std::enable_if<!std::is_move_constructible<U>::value, void *>::type
          _move(void *obj) const
          { throw std::runtime_error("not move constructible"); }

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
          typed_ptr(type const *type_,
                    bool const_,
                    void const *obj,
                    bool owning = false)
          : _type(type_),
            _const(const_),
            _obj(obj),
            _owning(owning)
          {}

          template<typename T>
          typed_ptr(T *obj, bool owning = false)
          : typed_ptr(type::get<T>(),
                      std::is_const<T>::value,
                      static_cast<void const *>(obj),
                      owning)
          {}

          ~typed_ptr()
          {
            if (_owning)
              _type->destroy(_obj);
          }

          void *copy() const
          {
            auto obj_copied = _type->copy(_obj);
            return static_cast<void *>(new typed_ptr(_type, false, obj_copied, true));
          }

          void *move() const
          {
            if (_const)
              return copy();

            auto obj_moved = _type->move(const_cast<void *>(_obj));
            return static_cast<void *>(new typed_ptr(_type, false, obj_moved, true));
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
          type const *_type;
          bool _const;
          void const *_obj;
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

        void *_copy(void *obj)
        { return get_typed(obj)->copy(); }

        // XXX copy assignment

        void *_move(void *obj)
        { return get_typed(obj)->move(); }

        // XXX move assignment

        void _delete(void *obj)
        { delete get_typed(obj); }
        """)


def _type_instances():
    types = OrderedDict()

    def add_type(t, t_bases=None):
        if t_bases is not None:
            t_bases = tuple(t_bases)

        types[(t.without_const(), t_bases)] = True

    for r in Record.ordering(): # XXX refactor
        add_type(r.type, (b.type for b in r.bases_recursive))

    for v in Backend().variables():
        if v.type.is_pointer() and not v.pointee().is_record():
            add_type(v.type.pointee())

    for f in Backend().functions():
        for t in [f.return_type()] + [p.type for p in f.parameters()]:
            if t.is_pointer() and not t.pointee().is_record():
                add_type(t.pointee())

    tis = []
    for t, t_bases in types.keys():
        template_params = [t]

        if t_bases is not None:
            template_params += t_bases

        template_params = ', '.join(map(str, template_params))

        tis.append(f"type_instance<{template_params}> {t.format(mangled=True)};")

    return '\n'.join(tis)
