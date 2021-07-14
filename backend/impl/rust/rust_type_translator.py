from cppbind import Identifier as Id, Type
from type_translator import TypeTranslator
from text import code


class RustTypeTranslator(TypeTranslator('rust')):
    rule = TypeTranslator('rust').rule

    _RUST_C_TYPE_MAP = {
        Type('bool'): 'c_int',
        Type('char'): 'c_char',
        Type('double'): 'c_double',
        Type('float'): 'c_float',
        Type('int'): 'c_int',
        Type('long long'): 'c_longlong',
        Type('long'): 'c_long',
        Type('short'): 'c_short',
        Type('signed char'): 'c_schar',
        Type('unsigned char'): 'c_uchar',
        Type('unsigned int'): 'c_uint',
        Type('unsigned long long'): 'c_ulonglong',
        Type('unsigned long'): 'c_ulong',
        Type('unsigned short'): 'c_ushort',
        Type('void'): 'c_void'
    }

    @classmethod
    def _c_type_fundamental(cls, t):
        c_type = cls._RUST_C_TYPE_MAP[t.unqualified().canonical()]

        return c_type

    @classmethod
    def _rust_type_fundamental(cls, t):
        if t.is_boolean():
            return 'bool'

        return cls._c_type_fundamental(t)

    @classmethod
    def _record(cls, t):
        r = t.as_record()

        return None if r is None else r.name_target()

    @classmethod
    def _record_trait(cls, t):
        r = t.as_record()

        return None if r is None or not r.is_polymorphic() else r.trait().name_target()

    @classmethod
    def _record_cast(cls, t, obj):
        r = t.pointee().as_record()

        cast = r.base_cast(const=t.pointee().is_const()).name().unqualified()

        if t.is_pointer():
            return f"(*{obj}).{cast}()"
        elif t.is_reference():
            return f"{obj}.{cast}()"

    @classmethod
    def _pointer_to(cls, t, what, dyn=False):
        if dyn:
            what = f"dyn {what}"

        if t.is_const():
            what = f"const {what}"
        else:
            what = f"mut {what}"

        return f"*{what}"

    @classmethod
    def _reference_to(cls, t, what, dyn=False):
        if dyn:
            what = f"dyn {what}"

        if not t.is_const():
            what = f"mut {what}"

        if cls._lifetime_counter is not None:
            what = f"&'_{cls._lifetime_counter} {what}"

            cls._lifetime_counter += 1
        else:
            what = f"&{what}"

        return what

    @rule(lambda t: t.is_pointer() or t.is_reference())
    def target_c(cls, t, args):
        return cls._pointer_to(t.pointee(), cls.target_c(t.pointee(), args))

    @rule(lambda t: t.is_record())
    def target_c(cls, t, args):
        return cls._record(t)

    @rule(lambda t: t.is_enum())
    def target_c(cls, t, args):
        return cls._c_type_fundamental(t.underlying_integer_type())

    @rule(lambda t: t.is_fundamental())
    def target_c(cls, t, args):
        return cls._c_type_fundamental(t)

    @rule(lambda t: t.is_void())
    def target(cls, t, args):
        return 'c_void'

    @rule(lambda t: t.is_record_indirection() and t.pointee().is_polymorphic())
    def target(cls, t, args):
        trait = cls._record_trait(t.pointee())

        if t.is_pointer():
            return cls._pointer_to(t.pointee(), trait, dyn=True)
        elif t.is_reference():
            return cls._reference_to(t.pointee(), trait, dyn=True)

    @rule(lambda t: t.is_c_string())
    def target(cls, t, args):
        return cls._reference_to(t.pointee(), "CStr")

    @rule(lambda t: t.is_pointer())
    def target(cls, t, args):
        return cls._pointer_to(t.pointee(), cls.target(t.pointee(), args))

    @rule(lambda t: t.is_reference())
    def target(cls, t, args):
        return cls._reference_to(t.referenced(), cls.target(t.referenced(), args))

    @rule(lambda t: t.is_record())
    def target(cls, t, args):
        return cls._record(t)

    @rule(lambda t: t.is_anonymous_enum())
    def target(cls, t, args):
        return cls._rust_type_fundamental(t.underlying_integer_type())

    @rule(lambda t: t.is_enum())
    def target(cls, t, args):
        enum = t.as_enum()

        if enum is None:
            return cls.target(t.underlying_integer_type())

        return t.as_enum().name_target()

    @rule(lambda t: t.is_fundamental())
    def target(cls, t, args):
        return cls._rust_type_fundamental(t)

    @rule(lambda t: t.is_c_string())
    def input(cls, t, args):
        return "{interm} = {inp}.as_ptr();"

    @rule(lambda t: t.is_record())
    def input(cls, t, args):
        return f"{{interm}} = {{inp}} as {cls.target_c(t.with_const().pointer_to(), args)};"

    @rule(lambda t: t.is_record_indirection() and t.pointee().is_polymorphic())
    def input(cls, t, args):
        if args.p.is_self():
            return f"{{interm}} = self as {cls.target_c(t, args)};"

        cast = cls._record_cast(t, '{inp}')

        if t.pointee().is_const():
            return code(
                f"""
                let {{interm}}_cast = {cast};
                {{interm}} = &{{interm}}_cast;
                """)
        else:
            return code(
                f"""
                let mut {{interm}}_cast = {cast};
                {{interm}} = &mut {{interm}}_cast;
                """)

    @rule(lambda t: t.is_record_indirection())
    def input(cls, t, args):
        if args.p.is_self():
            return f"{{interm}} = self as {cls.target_c(t, args)};"

        return "{interm} = {inp};"

    @rule(lambda t: t.is_enum())
    def input(cls, t, args):
        return f"{{interm}} = {{inp}} as {cls.target_c(t.underlying_integer_type(), args)};"

    @rule(lambda t: t.is_boolean())
    def input(cls, t, args):
        return f"{{interm}} = {{inp}} as {cls.target_c(t, args)};"

    @rule(lambda _: True)
    def input(cls, t, args):
        return "{interm} = {inp};"

    @rule(lambda t: t.is_c_string())
    def output(cls, t, args):
        return "CStr::from_ptr({outp})"

    @rule(lambda t: t.is_record_indirection())
    def output(cls, t, args):
        return "{outp}"

    @rule(lambda t: t.is_reference())
    def output(cls, t, args):
        if t.referenced().is_const():
            return "& *{outp}"
        else:
            return "&mut *{outp}"

    @rule(lambda t: t.is_enum())
    def output(cls, t, args):
        return "std::mem::transmute({outp})"

    @rule(lambda t: t.is_boolean())
    def output(cls, t, args):
        return f"{{outp}} != 0"

    @rule(lambda _: True)
    def output(cls, t, args):
        return "{outp}"

    @rule(lambda _: True)
    def helper(cls, t, args):
        pass
