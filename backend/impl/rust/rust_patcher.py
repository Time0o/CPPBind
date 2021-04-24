from backend import backend
from cppbind import Constant, EnumConstant, Identifier as Id, Type
from patcher import Patcher, _name
from text import code


def _type_target_c(self):
    return backend().type_translator().target_c(self)


def _type_target_rust(self):
    return backend().type_translator().target_rust(self)


def _type_target(self, which):
    if which == 'c':
        return self.target_c()
    elif which == 'rust':
        return self.target_rust()


def _constant_declare(self):
    return f"pub static {self.name_target()}: {self.type().unqualified().target_c()};"


def _constant_assign(self):
    return code(
        f"""
        pub unsafe fn get_{self.name_target(case=Id.SNAKE_CASE)}() -> {self.type().unqualified().target_rust()} {{
            c::{self.name_target()}
        }}
        """)


class RustPatcher(Patcher):
    def patch(self):
        self._patch(Constant, 'declare', _constant_declare)
        self._patch(Constant, 'assign', _constant_assign)
        self._patch(EnumConstant, 'name_target', _name(default_case=Id.PASCAL_CASE,
                                                       default_quals=Id.REMOVE_QUALS))
        self._patch(Type, 'target_c', _type_target_c)
        self._patch(Type, 'target_rust', _type_target_rust)
        self._patch(Type, 'target', _type_target)
