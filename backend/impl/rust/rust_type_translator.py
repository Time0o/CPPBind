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
        if t.is_alias():
            return t.format(case=Id.PASCAL_CASE, quals=Id.REMOVE_QUALS)

        if t.is_boolean():
            return 'bool'

        return cls._c_type_fundamental(t)

    @classmethod
    def _record(cls, t):
        return t.unqualified().format(with_template_postfix=True,
                                      case=Id.PASCAL_CASE,
                                      quals=Id.REMOVE_QUALS)

    @classmethod
    def _pointer_to(cls, t, what):
        if t.is_const():
            return f"* const {what}"
        else:
            return f"* mut {what}"

    @classmethod
    def _reference_to(cls, t, what):
        if t.is_const():
            return f"&'a {what}"
        else:
            return f"&'a mut {what}"

    @rule(lambda t: t.is_pointer() or t.is_reference())
    def target_c(cls, t, args):
        what = cls.target_c(t.pointee().unqualified(), args)
        return cls._pointer_to(t.pointee(), what)

    @rule(lambda t: t.is_record())
    def target_c(cls, t, args):
        return cls._record(t)

    @rule(lambda t: t.is_enum())
    def target_c(cls, t, args):
        return cls._c_type_fundamental(t.underlying_integer_type())

    @rule(lambda t: t.is_fundamental())
    def target_c(cls, t, args):
        return cls._c_type_fundamental(t)

    @rule(lambda t: t.is_alias() and t.is_basic())
    def target(cls, t, args):
        return t.format(case=Id.PASCAL_CASE, quals=Id.REMOVE_QUALS)

    @rule(lambda t: t.is_void())
    def target(cls, t, args):
        return "()"

    @rule(lambda t: t.is_c_string())
    def target(cls, t, args):
        return "&'a CStr"

    @rule(lambda t: t.is_pointer())
    def target(cls, t, args):
        what = cls.target(t.pointee().unqualified(), args)
        return cls._pointer_to(t.pointee(), what)

    @rule(lambda t: t.is_reference())
    def target(cls, t, args):
        what = cls.target(t.referenced().unqualified(), args)
        return cls._reference_to(t.referenced(), what)

    @rule(lambda t: t.is_record())
    def target(cls, t, args):
        return cls._record(t)

    @rule(lambda t: t.is_anonymous_enum())
    def target(cls, t, args):
        return cls._rust_type_fundamental(t.underlying_integer_type())

    @rule(lambda t: t.is_enum())
    def target(cls, t, args):
        return t.format(case=Id.PASCAL_CASE, quals=Id.REMOVE_QUALS)

    @rule(lambda t: t.is_fundamental())
    def target(cls, t, args):
        return cls._rust_type_fundamental(t)

    @rule(lambda t: t.is_c_string())
    def input(cls, t, args):
        return "{interm} = {inp}.as_ptr();"

    @rule(lambda t: t.is_record_indirection())
    def input(cls, t, args):
        if args.p.is_self():
            return f"{{interm}} = self as {cls.target_c(t, args)};"

        return "{interm} = {inp};"

    @rule(lambda t: t.is_record())
    def input(cls, t, args):
        return f"{{interm}} = {{inp}} as {cls.target_c(t.pointer_to(), args)};"

    @rule(lambda t: t.is_anonymous_enum())
    def input(cls, t, args):
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
