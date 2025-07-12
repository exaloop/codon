# Generates API reference for Codon standard library
# See CI workflow for usage

import itertools
import os
import os.path
import sys
import collections
import json

from pathlib import Path

# 1. Set up paths and load JSON
json_path = os.path.abspath(sys.argv[1])
out_path = os.path.abspath(sys.argv[2])
roots = [os.path.abspath(root) for root in sys.argv[3:]]
print(f"Generating documentation for {json_path} ...")
with open(json_path) as f:
    j = json.load(f)
print("Load done!")

# 2. Get the list of modules and create the documentation tree
modules = {k: v["path"] for k, v in j.items() if v["kind"] == "module"}
parsed_modules = collections.defaultdict(set)

def remove_root(path, roots):
    for root in roots:
        try:
            rel = str(Path(path).relative_to(root))
        except ValueError:
            continue
        return rel if rel != '.' else ''
    return path

for mid, module in modules.items():
    while module not in roots:
        directory, name = os.path.split(module)
        directory = remove_root(directory, roots)
        os.makedirs(os.path.join(out_path, directory), exist_ok=True)
        if name.endswith(".codon"):
            name = name[:-6]  # drop suffix
        parsed_modules[directory].add((name, mid))
        module = os.path.split(module)[0]

print("Module read done!")

for directory, modules in parsed_modules.items():
    module = directory.replace("/", ".")
    with open(f"{out_path}/{directory}/index.md", "w") as f:
        if module:
            print(f"# `{module}`\n", file=f)
        else:
            print("# Standard Library Reference\n", file=f)

        for m in sorted(set(m for m, _ in modules)):
            if m == "__init__":
                continue
            base = os.path.join('libraries', 'api', directory, m)
            is_dir = os.path.isdir(os.path.join(out_path, directory, m))
            print(f"- [`{m}`](/{base}{'/' if is_dir else ''})", file=f)

print(f" - Done with directory tree")

def parse_docstr(s, level=0):
    """Parse docstr s and indent it with level spaces"""
    s = s.split("\n")
    while s and s[0] == "":
        s = s[1:]
    while s and s[-1] == "":
        s = s[:-1]
    if not s:
        return ""
    i = 0
    indent = len(list(itertools.takewhile(lambda i: i == " ", s[0])))
    lines = [l[indent:] for l in s]
    return "\n".join(("   " * level) + l for l in lines)

def parse_type(a):
    """Parse type signature"""
    if isinstance(a, str) and not a.isdigit():
        return a
    elif isinstance(a, list):
        head, tail = a[0], a[1:]
    else:
        head, tail = a, None
    if head[0].isdigit() and head not in j:
        return "?"
    s = j[head]["name"] if head[0].isdigit() else head
    if tail:
        for ti, t in enumerate(tail):
            s += "[" if not ti else ", "
            s += parse_type(t)
        s += "]"
    return s

def parse_fn(v):
    """Parse function signature after the name"""
    s = ""
    if "generics" in v and v["generics"]:
        # don't include argument-generics
        generics_no_args = [
            g for g in v["generics"]
            if not any(a["name"] == g for a in v["args"])
        ]
        if generics_no_args:
            s += f'[{", ".join(v["generics"])}]'
    s += "("
    cnt = 0
    for ai, a in enumerate(v["args"]):
        s += "" if not cnt else ", "
        cnt += 1
        s += f'{a["name"] or "_"}'
        if "type" in a and a["type"]:
            s += ": " + parse_type(a["type"])
        if "default" in a:
            s += " = " + a["default"]
    s += ")"
    if "ret" in v:
        s += " -> " + parse_type(v["ret"])
    # if "extern" in v:
    #     s += f" (_{v['extern']} function_)"
    # s += "\n"
    return s

tag_tooltips = {
    "llvm": "Function is implemented with inline LLVM IR",
    "pure":
    "Function has no side effects and returns same value for same inputs",
    "nocapture":
    "Function does not capture arguments (return value might capture)",
    "derives": "Function return value captures arguments",
    "no_side_effect": "Function has no side effects",
    "self_captures": "Method's 'self' argument captures other arguments",
    "C": "Function is external C function",
    "overload": "Function is overloaded",
    "tuple": "Class is named tuple (cannot write fields)",
    "extend": "Class is extended to add given methods",
    "staticmethod": "Method is static (does not take 'self' argument)",
    "property": "Method is a class property",
    "associative": "Binary operator is associative",
    "commutative": "Binary operator is commutative",
    "distributive": "Binary operator is distributive",
    "inline": "Function always inlined",
    "noinline": "Function never inlined",
    "export": "Function is visible externally",
    "test": "Function is a test function",
    "__internal__": "Function is compiler-generated",
    "__attribute__": "Function is an attribute",
}

def write_tag(tag, f):
    tooltip = tag_tooltips.get(tag, "")
    if tooltip:
        f.write(f'  <span class="api-tag-tooltip">'
                f'     <span class="api-tag">@{tag}</span>'
                f'     <span class="api-tag-tooltiptext">{tooltip}</span>'
                f'  </span>')
    else:
        f.write(f'  <span class="api-tag">@{tag}</span>')

def write_tags(v, f):
    if "extern" in v:
        write_tag(v["extern"], f)
    if "attrs" in v and v["attrs"]:
        for attr in v["attrs"]:
            write_tag(attr, f)

def write_docstr(v, f):
    if "doc" in v:
        if "attrs" in v and "llvm" in v["attrs"]:
            f.write("\n``` llvm\n")
            f.write(parse_docstr(v["doc"]))
            f.write("\n```")
        else:
            f.write("\n")
            f.write(parse_docstr(v["doc"]))
            f.write("\n")
    f.write("\n")

# 3. Create documentation for each module
visited = set()
for directory, (name, mid) in {(d, m)
                               for d, mm in parsed_modules.items()
                               for m in mm}:
    if directory:
        module = f"{directory.replace('/', '.')}.{name}"
    else:
        module = name

    file, mode = f"{out_path}/{directory}/{name}.md", "w"

    if os.path.isdir(f"{out_path}/{directory}/{name}"):
        continue

    init = (name == "__init__")

    if init:
        file, mode = f"{out_path}/{directory}/index.md", "a"

    if file in visited:
        continue
    else:
        visited.add(file)

    with open(file, mode) as f:
        if not init:
            f.write(f"# module `{module}`\n")

        directory_prefix = directory + "/" if directory != "." else ""
        directory = directory.strip("/")
        dir_part = (directory + "/") if directory else ""
        f.write(f"\nSource: [`stdlib/{dir_part}{name}.codon`](https://github.com/exaloop/codon/blob/master/stdlib/{dir_part}{name}.codon)\n\n")

        write_docstr(j[mid], f)

        for i in j[mid]["children"]:
            v = j[i]

            if v["kind"] == "class" and v["type"] == "extension":
                v["name"] = j[v["parent"]]["name"]
            if v["name"].startswith("_"):
                continue

            icon = lambda name: f'<span style="color:#899499">:material-{name}:</span>'

            f.write("---\n")
            f.write("## ")
            if v["kind"] == "class":
                f.write(f'{icon("cube-outline")} **`{v["name"]}')
                if "generics" in v and v["generics"]:
                    f.write(f'[{",".join(v["generics"])}]')
                f.write("`**")
                if v["type"] == "extension":
                    write_tag("extend", f)
                elif v["type"] == "type":
                    write_tag("tuple", f)
            elif v["kind"] == "function":
                f.write(f'{icon("function")} **`{v["name"]}{parse_fn(v)}`**')
                write_tags(v, f)
            elif v["kind"] == "variable":
                f.write(f'{icon("variable")} **`{v["name"]}`**')
                if "type" in v:
                    f.write(f': `{parse_type(v["type"])}`')
                if "value" in v:
                    f.write(f' = `{v["value"]}`')

            write_docstr(v, f)

            if v["kind"] == "class":
                if "args" in v:
                    fields = [
                        c for c in v["args"] if not c["name"].startswith("_")
                    ]
                    if fields:
                        f.write("## Fields\n")
                        for c in fields:
                            f.write(f'### `{c["name"]}`')
                            if "type" in c:
                                f.write(f': `{parse_type(c["type"])}`\n')
                            f.write("\n")
                        f.write("\n")

                mt = [c for c in v["members"] if j[c]["kind"] == "function"]

                props = [c for c in mt if "property" in j[c].get("attrs", [])]
                if props:
                    print("## Properties\n", file=f)
                    for c in props:
                        v = j[c]
                        f.write(f'### `{v["name"]}`')
                        write_tags(v, f)
                        f.write("\n")
                        write_docstr(v, f)

                magics = [
                    c for c in mt
                    if len(j[c]["name"]) > 4 and j[c]["name"].startswith("__")
                    and j[c]["name"].endswith("__")
                ]
                if magics:
                    print("## Magic methods\n", file=f)
                    for c in magics:
                        v = j[c]
                        f.write(f'### `{v["name"]}{parse_fn(v)}`')
                        write_tags(v, f)
                        f.write("\n")
                        write_docstr(v, f)
                methods = [
                    c for c in mt if j[c]["name"][0] != "_" and c not in props
                ]
                if methods:
                    print("## Methods\n", file=f)
                    for c in methods:
                        v = j[c]
                        f.write(f'### `{v["name"]}{parse_fn(v)}`')
                        write_tags(v, f)
                        f.write("\n")
                        write_docstr(v, f)
            f.write("\n\n")

        f.write("\n\n")

print(" - Done with modules")
