import copy
import os

import text


class Path:
    def __init__(self, path):
        self._dirname, filename = \
            os.path.normpath(path).rsplit(os.path.sep, 1)

        self._filename, self._ext = os.path.splitext(filename)

    def path(self):
        return os.path.join(self._dirname, self._filename + self._ext)

    def dirname(self):
        return self._dirname

    def basename(self):
        return self._filename + self._ext

    def filename(self):
        return self._filename

    def ext(self):
        return self._ext

    def modified(self, dirname=None, filename=None, ext=None):
        new_path = copy.deepcopy(self)

        if dirname is not None:
            new_path._dirname = dirname.format(dirname=self._dirname)

        if filename is not None:
            new_path._filename = filename.format(filename=self._filename)

        if ext is not None:
            new_path._ext = ext.format(ext=self._ext)

        return new_path


class File:
    def __init__(self, path):
        self._content = []
        self._path = path

    def path(self):
        return self._path.path()

    def dirname(self):
        return self._path.dirname()

    def basename(self):
        return self._path.basename()

    def filename(self):
        return self._path.filename()

    def ext(self):
        return self._path.ext()

    def append(self, txt, **kwargs):
        self._content.append(txt, **kwargs)

    def prepend(self, txt, **kwargs):
        self._content.insert(0, txt, **kwargs)

    def write(self):
        try:
            with open(self._path.path(), 'w') as f:
                f.write(text.compress('\n'.join(self._content)))
        except Exception as e:
            raise ValueError(f"while dumping output file: {e}")
