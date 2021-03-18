from cppbind import Function, Identifier as Id, Type
from text import code


def _type_c_struct(self):
    return self.format(with_template_postfix=True,
                       with_prefix="struct ",
                       case=Id.SNAKE_CASE,
                       quals=Id.REPLACE_QUALS)


def _function_construct(self, parameters):
    cons = self.parent().type()

    return code(
        f"""
        new ({Id.OUT}->obj.mem) {cons}({parameters});
        {Id.OUT}->is_initialized = 1;
        {Id.OUT}->is_const = 0;
        {Id.OUT}->is_owning = 1;
        """)


def _function_destruct(self, this):
    des = self.parent().type().format(quals=Id.REMOVE_QUALS)

    return f"{this}->~{des}();"


Type.c_struct = _type_c_struct

Function.construct = _function_construct
Function.destruct = _function_destruct
