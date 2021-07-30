# Backend base class implementation. Each backend must provide a class deriving
# from 'Backend(...)' which implements a set of 'wrap_*' functions that control
# code generation.

from abc import ABCMeta, abstractmethod
from cppbind import Enum, Function, Options, Record, Type, Variable
from file import File, Path
from itertools import chain
import os


class BackendState:
    def __init__(self):
        self.impls = {} # Backend implementations
        self.insts = {} # Backend instantiations

        self.current_name = None # Current backend instance name
        self.current_inst = None # Current backend instance

_state = BackendState()


# Register a new backend.
def register_backend(be, impl):
    global _state

    _state.impls[be] = impl


# Initialize a new backend.
def initialize_backend(be, input_file, wrapper):
    global _state

    if be not in _state.impls:
        raise ValueError(f"backend '{be}' does not exist")

    _state.insts[be] = _state.impls[be](input_file, wrapper)


# Set current backend instance.
def set_backend_instance(be):
    global _state

    if be not in _state.insts:
        raise ValueError(f"backend '{be}' is not initialized")

    if _state.current_inst is not None:
        _state.current_inst.patcher().unpatch()

    _state.current_name = be
    _state.current_inst = _state.insts.get(be)

    _state.current_inst.patcher().patch()


# Get current backend instance.
def get_backend_instance(be='current'):
    global _state

    if be == 'current':
        be = _state.current_name
        inst = _state.current_inst
    else:
        inst = _state.insts.get(be)

    if inst is None:
        raise ValueError("no backend instance found")

    return be, inst


# Create a backend base class where 'be' is be the name of the backend. For
# example, the C backend should inherit from 'Backend('c')'.
def Backend(be):
    global _state

    # Metaclass magic that automatically registers backends once they've been
    # defined.
    class BackendMeta(ABCMeta):
        def __init__(cls, name, bases, clsdict):
            super().__init__(name, bases, clsdict)

            mro = cls.mro()

            # This is automatically executed whenever a class deriving from
            # 'Backend(...)' is defined.
            if len(mro) == 3:
                register_backend(be, mro[0])


    class BackendGeneric(metaclass=BackendMeta):
        def __init__(self, input_file, wrapper):
            self._input_file = Path(input_file)
            self._output_files = []

            self._includes = wrapper.includes()
            self._macros = wrapper.macros()

            self._enums = wrapper.enums()
            self._variables = wrapper.variables()
            self._functions = wrapper.functions()
            self._records = wrapper.records()

            self._objects = self._enums + \
                            self._variables + \
                            self._records + \
                            self._functions

            self._add_types()

            self._add_namespaces()

        # Construct a "namespace hierarchy" of all entities of interest in the
        # input translation unit with namespaces being modelled as nested
        # Python dictionaries. This is useful because it lets backends
        # implement their own scoping mechanisms in order to imitate namespaces
        # etc. An example is Lua where namespaces etc. are realized as tables.
        def _add_namespaces(self):
            self._namespaces = {}

            self._init_namespace(self._namespaces)

            self._namespaces['macros'] = self._macros

            for obj in self._objects:
                self._insert_into_namespace(self._namespaces, obj)

        def _init_namespace(self, ns):
            if 'namespaces' not in ns:
                ns['namespaces'] = {}

            keys = [
                'macros',
                'enums',
                'variables',
                'records',
                'functions'
            ]

            for key in keys:
                if key not in ns:
                    ns[key] = []

        def _insert_into_namespace(self, ns, obj):
            namespace = obj.namespace()
            if namespace is not None:
                for c in namespace.components():
                    if c not in ns['namespaces']:
                        ns['namespaces'][c] = {}

                    ns = ns['namespaces'][c]
                    self._init_namespace(ns)

            object_keys = [
                (Enum, 'enums'),
                (Variable, 'variables'),
                (Record, 'records'),
                (Function, 'functions')
            ]

            for t, key in object_keys:
                if isinstance(obj, t):
                    ns[key].append(obj)
                    break

        # Determine sets of types used in the input translation unit.
        def _add_types(self):
            self._types = set()
            self._type_aliases = set()
            self._type_aliases_target = None

            def add_type(t):
                while t.is_alias():
                    if t.is_basic():
                        self._type_aliases.add((t, t.canonical()))
                        break
                    elif t.is_const():
                        t = t.without_const()
                    elif t.is_pointer() or t.is_reference():
                        t = t.pointee()

                if not t.is_void():
                    self._types.add(t.unqualified())

                if t.is_enum():
                    add_type(t.underlying_integer_type())
                elif t.is_pointer() or t.is_reference():
                    while t.is_pointer() or t.is_reference():
                        if t.pointee().is_void():
                            self._types.add(t.pointee().unqualified())
                            break
                        else:
                            t = t.pointee()
                            self._types.add(t.unqualified())

            for v in self.variables(include_macros=True, include_enums=True):
                add_type(v.type())

            for r in self.records():
                add_type(r.type())

            for f in chain(self._functions, *(r.functions() for r in self.records())):
                for t in [f.return_type()] + [p.type() for p in f.parameters()]:
                    add_type(t)

        @abstractmethod
        def patcher(self):
            pass

        @abstractmethod
        def type_translator(self):
            pass

        def input_file(self):
            return self._input_file

        def input_includes(self, relative=None):
            if relative is None:
                relative = Options.output_relative_includes

            return [inc.str(relative=relative) for inc in self._includes]

        def output_file(self, output_path):
            output_dir = os.path.abspath(Options.output_directory)

            output_file = File(output_path.modified(dirname=output_dir))

            self._output_files.append(output_file)

            return output_file

        def namespaces(self):
            return self._namespaces

        def types(self):
            return sorted(self._types)

        def type_aliases(self):
            if self._type_aliases_target is None:
                self._type_aliases_target = set()
                for a, t in self._type_aliases:
                    if a.target() != t.target():
                        self._type_aliases_target.add((a, t))

                self._type_aliases_target = list(sorted(self._type_aliases_target))

            return self._type_aliases_target

        def macros(self):
            return self._macros

        def enums(self):
            return self._enums

        def variables(self, include_macros=False, include_enums=False):
            variables = self._variables[:]

            if include_macros:
                variables += [m.as_variable() for m in self._macros]

            if include_enums:
                for e in self._enums:
                    variables += [c.as_variable() for c in e.constants()]

            return variables

        def functions(self, include_members=False):
            functions = self._functions[:]

            if include_members:
                for r in self.records():
                    if not r.is_abstract():
                        functions += r.functions()

            return functions

        def records(self, include_declarations=False, include_abstract=True):
            records = []

            for r in self._records:
                if not include_declarations and not r.is_definition():
                    continue

                if not include_abstract and r.is_abstract():
                    continue

                records.append(r)

            return records

        # Obtain list of records types. If which == 'defined', only include those
        # for which definitions are available, if which == 'used' only include
        # those for which only declarations are available.
        def record_types(self, which='all'):
            types_all = set()
            types_defined = set()

            def add_record_type(t, which):
                if which == 'all':
                    types_all.add(t)
                elif which == 'defined':
                    types_defined.add(t)

            for t in self.types():
                if t.is_alias():
                    t = t.canonical()

                if t.is_record():
                    add_record_type(t.without_const(), 'all')
                elif t.is_record_indirection():
                    add_record_type(t.pointee().without_const(), 'all')

            for r in self.records():
                add_record_type(r.type(), 'defined')

            types_used = types_all - types_defined

            if which == 'all':
                types = types_all
            elif which == 'defined':
                types = types_defined
            elif which == 'used':
                types = types_used

            return sorted(types)

        def run(self):
            self.wrap_before()

            for m in self._macros:
                self.wrap_macro(m)

            for e in self._enums:
                self.wrap_enum(e)

            for c in self._variables:
                self.wrap_variable(c)

            for r in self._records:
                self.wrap_record(r)

            for f in self._functions:
                self.wrap_function(f)

            self.wrap_after()

            for output_file in self._output_files:
                output_file.write()

        def wrap_before(self):
            pass

        def wrap_after(self):
            pass

        def wrap_macro(self, m):
            self.wrap_variable(m.as_variable())

        def wrap_enum(self, e):
            for c in e.constants():
                self.wrap_variable(c.as_variable())

        @abstractmethod
        def wrap_variable(self, v):
            pass

        @abstractmethod
        def wrap_function(self, f):
            pass

        @abstractmethod
        def wrap_record(self, r):
            pass

    return BackendGeneric


# Get current backend instance.
def backend():
    _, inst = get_backend_instance()
    return inst


# Switch to a certain backend.
def use_backend(be):
    set_backend_instance(be)


def run_backend(be):
    use_backend(be)

    return backend().run()


# Same as 'use_backend' but wrapped in a context manager.
class switch_backend:
    def __init__(self, be):
        self._previous_be, _ = get_backend_instance()
        self._current_be = be

    def __enter__(self):
        set_backend_instance(self._current_be)

    def __exit__(self, type, value, traceback):
        set_backend_instance(self._previous_be)
