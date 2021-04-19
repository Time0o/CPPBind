import c_util
from cppbind import Identifier as Id, Options
from functools import partial
from text import code
from type_translator import TypeTranslator


class CTypeTranslator(TypeTranslator('c')):
    rule = TypeTranslator('c').rule

    @classmethod
    def _c_type(cls, t):
        fmt = partial(t.format,
                      with_template_postfix=True,
                      case=Id.SNAKE_CASE,
                      quals=Id.REPLACE_QUALS)

        if t.is_alias():
            fmt = partial(fmt, with_extra_postfix='_t')
        elif t.is_record() or t.is_record_indirection():
            fmt = partial(fmt, with_extra_prefix='struct ')

        return fmt()

    @classmethod
    def _c_input_assert(cls, inp, assertions):
        return code(
            """
            {assertions}
            {{interm}} = {inp};
            """,
            assertions='\n'.join(assertions),
            inp=inp)

    @rule(lambda t: t.is_alias() and t.is_basic())
    def target(cls, t, args):
        return cls._c_type(t)

    @rule(lambda t: t.is_boolean())
    def target(cls, t, args):
        return 'int'

    @rule(lambda t: t.is_enum())
    def target(cls, t, args):
        return cls._c_type(t.underlying_integer_type())

    @rule(lambda t: t.is_pointer() and t.pointee(recursive=True).is_fundamental())
    def target(cls, t, args):
        return cls._c_type(t)

    @rule(lambda t: t.is_reference() and t.referenced().is_fundamental())
    def target(cls, t, args):
        return cls._c_type(t.referenced().pointer_to())

    @rule(lambda t: t.is_record())
    def target(cls, t, args):
        return cls._c_type(t)

    @rule(lambda t: t.is_record_indirection())
    def target(cls, t, args):
        return cls._c_type(t.pointee().pointer_to())

    @rule(lambda t: t.is_pointer() or t.is_reference() and \
                    t.pointee().is_record_indirection())
    def target(cls, t, args):
        return cls._c_type(t.pointee().pointee().pointer_to())

    @rule(lambda t: t.is_pointer() or t.is_reference())
    def target(cls, t, args):
        raise ValueError("unsupported target pointer/reference type {}".format(t))

    @rule(lambda _: True)
    def target(cls, t, args):
        return cls._c_type(t)

    @rule(lambda t: t.is_enum())
    def constant(cls, t, args):
        return f"{t.target()} {args.c.name_target()} = static_cast<{t.underlying_integer_type()}>({{const}});"

    @rule(lambda _: True)
    def constant(cls, t, args):
        return f"{t.target()} {args.c.name_target()} = {{const}};"

    @rule(lambda t: t.is_enum())
    def input(cls, t, args):
        return f"{{interm}} = static_cast<{t}>({{inp}});"

    @rule(lambda t: t.is_pointer() and t.pointee(recursive=True).is_fundamental() or \
                    t.is_reference() and t.referenced().is_fundamental())
    def input(cls, t, args):
        return "{interm} = {inp};"

    @rule(lambda t: t.is_record())
    def input(cls, t, args):
        return cls._c_input_assert(c_util.struct_cast(t, '{inp}'),
                                   ["assert({inp}->is_initialized);"])

    @rule(lambda t: t.is_record_indirection())
    def input(cls, t, args):
        assertions = []

        if args.f.is_destructor():
            assertions.append("if (!{inp}->is_initialized) return;")
        else:
            assertions.append("assert({inp}->is_initialized);")

        if not t.pointee().is_const() and not args.f.is_destructor():
            assertions.append("assert(!{inp}->is_const);")

        return cls._c_input_assert(c_util.struct_cast(t.pointee(), '{inp}'),
                                   assertions)

    @rule(lambda t: t.is_pointer() or t.is_reference() and \
                    t.pointee().is_record_indirection())
    def input(cls, t, args):
        return f"{{interm}} = &{c_util.struct_cast(t, '{inp}')};"

    @rule(lambda t: t.is_pointer() or t.is_reference())
    def input(cls, t, args):
        raise ValueError("unsupported input pointer/reference type {}".format(t))

    @rule(lambda _: True)
    def input(cls, t, args):
        return "{interm} = {inp};"

    @rule(lambda t: t.is_void())
    def output(cls, t, args):
        return

    @rule(lambda t: t.is_enum())
    def output(cls, t, args):
        return f"return static_cast<{t.underlying_integer_type()}>({{outp}});"

    @rule(lambda t: t.is_pointer() and t.pointee(recursive=True).is_fundamental() or \
                    t.is_reference() and t.referenced().is_fundamental())
    def output(cls, t, args):
        return "return {outp};"

    @rule(lambda t: t.is_record())
    def output(cls, t, args):
        return code(
            f"""
            {cls._c_type(t)} {Id.OUT};
            {c_util.make_owning_struct_args(f'&{Id.OUT}', t, '{outp}')};
            return {Id.OUT};
            """)

    @rule(lambda t: t.is_record_indirection())
    def output(cls, t, args):
        if args.f.is_constructor():
            return code(
                f"""
                static_cast<void>({{outp}});

                {cls._c_type(t.pointee())} {Id.OUT};
                {c_util.make_owning_struct_mem(f'&{Id.OUT}', t.pointee(), Id.TMP)};
                return {Id.OUT};
                """)
        else:
            return code(
                f"""
                {cls._c_type(t.pointee().without_const())} {Id.OUT};
                {c_util.make_non_owning_struct(f'&{Id.OUT}', '{outp}')};
                return {Id.OUT};
                """)

    @rule(lambda t: t.is_pointer() or t.is_reference())
    def output(cls, t, args):
        raise ValueError("unsupported output pointer/reference type {}".format(t))

    @rule(lambda _: True)
    def output(cls, t, args):
        return "return {outp};"

    def exception(cls, args):
        ex = "errno = EBIND;"

        if args.f.out_type() is None and not args.f.return_type().is_void():
            ex = code(
                """
                errno = EBIND;
                return {{}};
                """)

        return ex
