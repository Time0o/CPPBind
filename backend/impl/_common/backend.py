import abc

import file

import cppbind


class BackendMeta(abc.ABCMeta):
    def __init__(cls, name, bases, clsdict):
        if len(cls.mro()) == 3:
            BackendMeta.BackendImpl = cls

        super().__init__(name, bases, clsdict)


class Backend(metaclass=BackendMeta):
    def __init__(self, input_path, records, functions, options):
        self._input_path = file.Path(input_path)
        self._output_files = []

        self._records = records
        self._functions = functions

        self._options = options

    def run(self):
        self.wrap_before()

        for f in self._functions:
            self.wrap_function(f)

        self.wrap_after()

        for output_file in self._output_files:
            output_file.write()

    def input_path(self):
        return self._input_path

    def output_file(self, output_path):
        output_dir = self.option('output-directory')

        if output_dir:
            output_path = output_path.modified(dirname=output_dir)

        output_file = file.File(output_path)

        self._output_files.append(output_file)

        return output_file

    def records(self):
        return self._records

    def functions(self):
        return self._functions

    def option(self, name):
        try:
            getter = getattr(self._options, name)
        except AttributeError:
            raise ValueError(f"no such option: '{name}'")

        return getter()

    @abc.abstractmethod
    def wrap_before(self):
        pass

    @abc.abstractmethod
    def wrap_after(self):
        pass

    @abc.abstractmethod
    def wrap_function(self, f):
        pass


def run(*args):
    return BackendMeta.BackendImpl(*args).run()
