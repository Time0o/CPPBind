import patch

from backend import Backend
from type_translator import TypeTranslator


def run(*args, **kwargs):
    Backend(*args, *kwargs)
    TypeTranslator()

    return Backend().run()
