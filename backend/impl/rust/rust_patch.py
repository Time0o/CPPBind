from backend import backend
from cppbind import Type
from patch import Patcher


def _type_target_c(self):
    return backend().type_translator().target_c(self)


def _type_target_rust(self):
    return backend().type_translator().target_rust(self)


def _type_target(self, which):
    if which == 'c':
        return self.target_c()
    elif which == 'rust':
        return self.target_rust()


class RustPatcher(Patcher):
    def patch(self):
        self._type_target_c_orig = getattr(Type, 'target_c', None)
        self._type_target_rust_orig = getattr(Type, 'target_rust', None)
        self._type_target_orig = Type.target

        Type.target_c = _type_target_c
        Type.target_rust = _type_target_rust
        Type.target = _type_target

    def unpatch(self):
        Type.target_c = self._type_target_c_orig
        Type.target_rust = self._type_target_rust_orig
        Type.target = self._type_target_orig
