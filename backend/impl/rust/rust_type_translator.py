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
    def _rust_c_type(cls, t, args):
        c_type = cls._RUST_C_TYPE_MAP[t.unqualified()]

        if args.scoped:
            c_type = f"c::{c_type}"

        return c_type

    @rule(lambda t: t.is_pointer() or t.is_reference())
    def target_c(cls, t, args):
        ptr = cls.target_c(t.pointee().unqualified(), args)

        if t.pointee().is_const():
            return f"* const {ptr}"
        else:
            return f"* mut {ptr}"

    @rule(lambda t: t.is_record())
    def target_c(cls, t, args):
        return t.unqualified().format(case=Id.PASCAL_CASE, quals=Id.REMOVE_QUALS)

    @rule(lambda t: t.is_enum())
    def target_c(cls, t, args):
        return cls._rust_c_type(t.underlying_integer_type(), args)

    @rule(lambda t: t.is_fundamental())
    def target_c(cls, t, args):
        return cls._rust_c_type(t, args)

    @rule(lambda t: t.is_c_string())
    def target(cls, t, args):
        return "&'a c::CStr"

    @rule(lambda t: t.is_pointer())
    def target(cls, t, args):
        return t.target_c()

    @rule(lambda t: t.is_reference())
    def target(cls, t, args):
        ref = t.referenced().target_c()

        if t.referenced().is_const():
            return f"&'a {ref}"
        else:
            return f"&'a mut {ref}"

    @rule(lambda t: t.is_record())
    def target(cls, t, args):
        return t.target_c()

    @rule(lambda t: t.is_anonymous_enum())
    def target(cls, t, args):
        return t.underlying_integer_type().target_c()

    @rule(lambda t: t.is_enum())
    def target(cls, t, args):
        return t.format(case=Id.PASCAL_CASE, quals=Id.REMOVE_QUALS)

    @rule(lambda t: t.is_boolean())
    def target(cls, t, args):
        return 'bool'

    @rule(lambda t: t.is_fundamental())
    def target(cls, t, args):
        return t.target_c()

    @rule(lambda t: t.is_record_indirection())
    def input(cls, t, args):
        if args.p.is_self():
            return f"{{interm}} = self as {t.target_c()};"

        return "{interm} = {inp};"

    @rule(lambda t: t.is_record())
    def input(cls, t, args):
        return f"{{interm}} = {{inp}} as {t.pointer_to().target_c()};"

    @rule(lambda t: t.is_c_string())
    def input(cls, t, args):
        return "{interm} = {inp}.as_ptr();"

    @rule(lambda t: t.is_anonymous_enum())
    def input(cls, t, args):
        return "{interm} = {inp};"

    @rule(lambda t: t.is_enum())
    def input(cls, t, args):
        return f"{{interm}} = {{inp}} as {t.underlying_integer_type().target_c()};"

    @rule(lambda t: t.is_boolean())
    def input(cls, t, args):
        return f"{{interm}} = {{inp}} as {t.target_c()};"

    @rule(lambda _: True)
    def input(cls, t, args):
        return "{interm} = {inp};"

    @rule(lambda t: t.is_c_string())
    def output(cls, t, args):
        return "c::CStr::from_ptr({outp})"

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
