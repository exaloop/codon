# Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

from . import decorator

import inspect
import os
import sys
import types
import importlib.abc
import importlib.machinery


class ImportFinder(importlib.abc.MetaPathFinder):
    def __init__(self, loader):
        self._loader = loader
    def find_spec(self, fullname, path, target=None):
        if self._loader.provides(fullname):
            return self._gen_spec(fullname)
    def _gen_spec(self, fullname):
        return importlib.machinery.ModuleSpec(fullname, self._loader)


class ImportLoader(importlib.abc.Loader):
    def __init__(self):
        self._imports = {}
    def provide(self, name, module):
        self._imports[name] = module
    def provides(self, fullname):
        return fullname in self._imports
    def create_module(self, spec):
        if spec.name not in self._imports:
            raise ValueError(f"cannot find {spec.name}")
        return self._imports[spec.name]
    def exec_module(self, module):
        pass


class Numpy:
    def __init__(self):
        self.__dict__ = __import__("numpy").__dict__


def codonize(fname, lines):
    code = '\n'.join(lines)
    decorator._jit.execute(code, fname, 1, 0)


def pythonize(lines):
    exec('\n'.join(lines))


if __name__ == "__main__":
    path = os.path.realpath(sys.argv[1])
    with open(path) as f:
        lines = list(x.rstrip() for x in f.readlines())
    try:
        codonize(path, lines)
    except:
        print("[codon] codonization failed, moving to python")
        loader = ImportLoader()
        sys.meta_path.append(ImportFinder(loader))

        # Add custom plugins
        loader.provide("compy", Numpy())

        pythonize(lines)
    finally:
        print("[codon] done")
