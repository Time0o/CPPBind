import backend
import patch


def run(*args, **kwargs):
    return backend.Backend(*args, **kwargs).run()
