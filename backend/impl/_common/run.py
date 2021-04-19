from backend import backend, use_backend


def run(be):
    use_backend(be)

    return backend().run()
