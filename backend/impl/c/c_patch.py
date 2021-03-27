import c_util
from cppbind import Function, Identifier as Id, Type
from text import code


def _type_is_record_ref(self):
    return (self.is_pointer() or self.is_reference()) and self.pointee().is_record()


def _type_c_struct(self):
    return self.format(with_template_postfix=True,
                       with_prefix="struct ",
                       case=Id.SNAKE_CASE,
                       quals=Id.REPLACE_QUALS)


def _function_before_call(self):
    rt = self.return_type()
    if rt.is_record() or rt.is_record_ref():
        if rt.is_record():
            t = rt
        elif rt.is_record_ref():
            t = rt.pointee()

        t = t.without_const()

        return f"{t.c_struct()} {Id.OUT};"


def _function_construct(self, parameters):
    t = self.parent().type()

    return code(
        f"""
        {Id.OUT};
        {c_util.init_owning_struct(f'&{Id.RET}', t, parameters)};
        """)


def _function_destruct(self, this):
    t = self.parent().type()

    return f"{this}->~{t.format(quals=Id.REMOVE_QUALS)}();"


Type.is_record_ref = _type_is_record_ref
Type.c_struct = _type_c_struct

Function.before_call = _function_before_call
Function.construct = _function_construct
Function.destruct = _function_destruct
