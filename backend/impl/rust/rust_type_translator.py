from cppbind import Type
from type_translator import TypeTranslator
from text import code


class RustTypeTranslator(TypeTranslator('rust')):
    rule = TypeTranslator('rust').rule

    _rust_c_type_map = {
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
    def _rust_c_type(cls, t, scoped=False):
        c_type = cls._rust_c_type_map[t.without_const()]

        if scoped:
            c_type = f"c::{c_type}"

        if t.is_const():
            c_type = f"const {c_type}"

        return c_type

    @rule(lambda t: t.is_pointer())
    def target_c(cls, t, args):
        return f"* {cls.target_c(t.pointee())}"

    @rule(lambda t: t.is_enum())
    def target_c(cls, t, args):
        return cls._rust_c_type(t.underlying_integer_type())

    @rule(lambda t: t.is_fundamental())
    def target_c(cls, t, args):
        return cls._rust_c_type(t)

    @rule(lambda t: t.is_c_string())
    def target_rust(cls, t, args):
        return "&'static str"

    @rule(lambda t: t.is_enum())
    def target_rust(cls, t, args):
        return cls._rust_c_type(t.underlying_integer_type(), scoped=True)

    @rule(lambda t: t.is_fundamental())
    def target_rust(cls, t, args):
        return cls._rust_c_type(t, scoped=True)

    @rule(lambda t: t.is_c_string())
    def output(cls, t, args):
        return code(
            """
            use std::ffi::CStr;
            CStr::from_ptr({outp}).to_str().unwrap()
            """)

    @rule(lambda _: True)
    def output(cls, t, args):
        return "{outp}"
