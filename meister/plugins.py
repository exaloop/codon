# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Codon plugin manifests, stdlib search paths, and runtime libraries."""

import operator
import re
import sys
import tomllib
from ctypes import CDLL, RTLD_GLOBAL
from dataclasses import dataclass
from pathlib import Path


class PluginError(RuntimeError):
    pass


COMPARATORS = {
    "=": operator.eq, "!=": operator.ne, "<": operator.lt, "<=": operator.le,
    ">": operator.gt, ">=": operator.ge,
}
COMPONENT = r"(?:0|[1-9][0-9]*|[xX*])"
IDENTIFIERS = r"[0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*"
RANGE_TERM = re.compile(
    rf"(?:(<=|>=|!=|[=<>~^])\s*)?"
    rf"({COMPONENT}(?:\.{COMPONENT}){{0,2}})"
    rf"(?:-({IDENTIFIERS}))?(?:\+{IDENTIFIERS})?(?=\s|$)"
)


def supports_version(expression, version):
    # C++ uses semver::try_parse_range. The compiler version is a stable release.
    def compare(term):
        op, core, prerelease = term.groups()
        parts = core.split(".")
        numbers = tuple(int(part) for part in parts if part.isdecimal())
        if len(numbers) != len(parts) and (
            op or any(part.isdecimal() for part in parts[len(numbers):])
        ):
            raise ValueError
        if (prerelease or "+" in term[0]) and len(numbers) != 3:
            raise ValueError
        if prerelease and any(
            part.isdecimal() and len(part) > 1 and part[0] == "0"
            for part in prerelease.split(".")
        ):
            raise ValueError
        if not numbers:
            return True
        bound = numbers + (0,) * (3 - len(numbers))
        # A stable version sorts after any prerelease with the same core.
        current, requested = (*version, True), (*bound, prerelease is None)
        if op in COMPARATORS:
            if len(numbers) < 3 and op not in (">=", "<"):
                raise ValueError
            return COMPARATORS[op](current, requested)
        if not op and len(numbers) == 3:
            return current == requested
        if op == "^":
            index = next((i for i, number in enumerate(numbers) if number), len(numbers) - 1)
        else:
            index = min(len(numbers) - 1, 1)
        upper = (*bound[:index], bound[index] + 1, *((0,) * (2 - index)))
        return current >= requested and version < upper

    matches = []
    for alternative in expression.split("||"):
        alternative = alternative.strip()
        if not alternative:
            raise ValueError
        included = True
        while alternative:
            term = RANGE_TERM.match(alternative)
            if not term:
                raise ValueError
            included = compare(term) and included
            alternative = alternative[term.end():].lstrip()
        matches.append(included)
    return any(matches)


@dataclass(frozen=True)
class Plugin:
    root: Path
    stdlib: Path | None
    runtime: Path | None
    link_args: tuple[str, ...]


class PluginManager:
    def __init__(self, filesystem, version):
        self.fs = filesystem
        self.version = version
        self.plugins = {}
        self.runtime_libraries = {}

    def load(self, path):
        root = Path(path)
        manifest = root / "plugin.toml"
        if not self.fs.exists(manifest) and not root.is_absolute():
            directory = self.fs.executable_path(self.fs.argv0).parent
            installed = directory / "plugins" / root
            root = installed if self.fs.exists(installed / "plugin.toml") else (
                directory / "../lib/codon/plugins" / root
            )
            manifest = root / "plugin.toml"
        root = self.fs.canonical(root)
        if root in self.plugins:
            return self.plugins[root]

        try:
            with manifest.open("rb") as stream:
                config = tomllib.load(stream)
        except (OSError, ValueError) as error:
            raise PluginError(f'[toml::parse_file("{manifest}")] {error}') from error

        def field(table, name):
            value = table.get(name)
            return value if isinstance(value, str) else ""

        about, library = config.get("about"), config.get("library")
        about = about if isinstance(about, dict) else {}
        library = library if isinstance(library, dict) else {}
        for key in ("compiler", "runtime"):
            if key in library and not isinstance(library[key], str):
                raise PluginError(f"library.{key} must be a string")

        supported = field(about, "supported")
        if supported:
            try:
                compatible = supports_version(supported, self.version)
            except ValueError as error:
                raise PluginError(
                    f'[semver::try_parse_range("{supported}")] invalid range'
                ) from error
            if not compatible:
                version = ".".join(map(str, self.version))
                raise PluginError(f"unsupported version {version} (supported: {supported})")

        suffix = {"darwin": ".dylib", "win32": ".dll"}.get(sys.platform, ".so")
        legacy = field(library, "cpp")
        compiler = library.get("compiler", legacy)
        if compiler:
            raise PluginError(f"native C++ compiler plugins require the Codon compiler ABI ({root / (compiler + suffix)})")

        link = library.get("link")
        runtime = library.get("runtime", "" if link is False else legacy)
        arguments = link if isinstance(link, list) else [link]
        try:
            link_args = tuple(
                argument.format(root=str(root)) for argument in arguments
                if isinstance(argument, str) and argument
            )
        except (IndexError, KeyError, ValueError) as error:
            raise PluginError(f"invalid library.link in {manifest}: {error}") from error
        stdlib = field(library, "codon")
        plugin = Plugin(
            root, root / stdlib if stdlib else None,
            root / (runtime + suffix) if runtime else None, link_args,
        )
        if plugin.stdlib:
            self.fs.add_search_path(str(plugin.stdlib))
        self.plugins[root] = plugin
        return plugin

    def load_runtime_libraries(self):
        for plugin in self.plugins.values():
            path = plugin.runtime
            if path is not None and path not in self.runtime_libraries:
                try:
                    self.runtime_libraries[path] = CDLL(str(path), mode=RTLD_GLOBAL)
                except OSError as error:
                    raise PluginError(f"could not load runtime library '{path}': {error}") from error

    def add_link_paths(self, command):
        for plugin in self.plugins.values():
            if plugin.runtime:
                directory = plugin.runtime.parent
                command.append(f"-L{directory}")
                if sys.platform != "win32":
                    command.append(f"-Wl,-rpath,{directory}")

    def add_link_libraries(self, command):
        for plugin in self.plugins.values():
            if plugin.link_args:
                command.extend(plugin.link_args)
            elif plugin.runtime:
                if sys.platform == "win32":
                    command.append(str(plugin.runtime.with_suffix(".lib")))
                else:
                    command.append("-l" + plugin.runtime.stem.removeprefix("lib"))
