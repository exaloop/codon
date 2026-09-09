# Generates API reference for Codon standard library
# See CI workflow for usage

import collections
import html
import json
import os
import re
import sys
import markdown
import textwrap

from pathlib import Path
from importlib.resources import files


# =============================================================================
# Configuration
# =============================================================================

GITHUB_STDLIB_URL = "https://github.com/exaloop/codon/blob/master/stdlib"

TAG_TOOLTIPS = {
    "llvm": "Implemented with inline LLVM IR",
    "pure": "Has no side effects and returns the same value for the same inputs",
    "nocapture": "Does not capture arguments; the return value may capture",
    "derives": "Return value captures arguments",
    "no_side_effect": "Has no side effects",
    "self_captures": "The self argument captures other arguments",
    "C": "External C function",
    "overload": "Overloaded function",
    "tuple": "Named tuple; fields cannot be modified",
    "extend": "Extends an existing class",
    "staticmethod": "Static method",
    "property": "Property",
    "associative": "Associative binary operator",
    "commutative": "Commutative binary operator",
    "distributive": "Distributive binary operator",
    "inline": "Always inlined",
    "noinline": "Never inlined",
    "export": "Visible externally",
    "test": "Test function",
    "__internal__": "Compiler-generated",
    "__attribute__": "Compiler attribute",
}


# =============================================================================
# Input
# =============================================================================

if len(sys.argv) < 4:
    raise SystemExit(
        "usage: generate_api.py API.json OUTPUT_DIR ROOT [ROOT ...]"
    )

json_path = os.path.abspath(sys.argv[1])
out_path = os.path.abspath(sys.argv[2])
roots = [os.path.abspath(root) for root in sys.argv[3:]]

print(f"Generating documentation for {json_path} ...")

with open(json_path) as f:
    j = json.load(f)

print("Load done!")


# =============================================================================
# General helpers
# =============================================================================

def load_material_icon(path):
    """
    Load an SVG icon bundled with Material for MkDocs.

    Example:
        load_material_icon("fontawesome/brands/github")
    """
    icon = (
        files("material")
        / "templates"
        / ".icons"
        / f"{path}.svg"
    )

    return icon.read_text(encoding="utf-8")


GITHUB_ICON = load_material_icon(
    "fontawesome/brands/github"
)


def escape(value):
    return html.escape(str(value), quote=True)


def normalize_path(path):
    return path.replace("\\", "/").strip("/")


def relative_to_any_root(path):
    path = Path(path)

    for root in roots:
        try:
            return normalize_path(str(path.relative_to(root)))
        except ValueError:
            pass

    return normalize_path(str(path))


def slug_component(value):
    """
    Create an anchor-safe component while preserving API names reasonably well.
    """
    value = re.sub(r"[^a-zA-Z0-9_-]+", "-", value)
    return value.strip("-").lower() or "symbol"


def is_public(v):
    name = v.get("name", "")
    return bool(name) and not name.startswith("_")


def is_magic_method(v):
    name = v.get("name", "")

    return (
        len(name) > 4
        and name.startswith("__")
        and name.endswith("__")
    )


# =============================================================================
# Types and signatures
# =============================================================================

def parse_type(value):
    if value is None:
        return "?"

    if isinstance(value, str) and not value.isdigit():
        return value

    if isinstance(value, list):
        if not value:
            return "?"

        head = value[0]
        tail = value[1:]
    else:
        head = value
        tail = []

    head = str(head)

    if head and head[0].isdigit():
        if head not in j:
            return "?"
        result = j[head]["name"]
    else:
        result = head

    if tail:
        result += "["
        result += ", ".join(parse_type(item) for item in tail)
        result += "]"

    return result


def parse_fn(v):
    result = ""

    args = v.get("args", [])
    generics = v.get("generics", [])

    if generics:
        argument_names = {
            arg.get("name")
            for arg in args
            if arg.get("name")
        }

        standalone_generics = [
            generic
            for generic in generics
            if generic not in argument_names
        ]

        if standalone_generics:
            result += "[" + ", ".join(standalone_generics) + "]"

    result += "("

    arguments = []

    for arg in args:
        part = arg.get("name") or "_"

        if arg.get("type"):
            part += ": " + parse_type(arg["type"])

        if "default" in arg:
            part += " = " + str(arg["default"])

        arguments.append(part)

    result += ", ".join(arguments)
    result += ")"

    if v.get("ret"):
        result += " -> " + parse_type(v["ret"])

    return result


def class_signature(v, display_name):
    generics = v.get("generics", [])

    if generics:
        return f'class {display_name}[{", ".join(generics)}]'

    return f"class {display_name}"


# =============================================================================
# Anchors
# =============================================================================

def class_anchor(name):
    return f"class-{slug_component(name)}"


def function_anchor(name):
    return f"function-{slug_component(name)}"


def variable_anchor(name):
    return f"variable-{slug_component(name)}"


def member_anchor(owner_anchor, name):
    return f"{owner_anchor}-{slug_component(name)}"


def write_permalink(anchor, f, label):
    """
    Custom API permalink. This does not depend on MkDocs heading anchors.
    """
    f.write(
        f'<a class="api-permalink" '
        f'href="#{escape(anchor)}" '
        f'aria-label="Link to {escape(label)}" '
        f'title="Copy/link to this API element">'
        '<svg viewBox="0 0 24 24" aria-hidden="true">'
        '<path d="M10.59 13.41a2 2 0 0 0 2.82 0l3.59-3.59a2 2 0 1 0-2.82-2.82l-1.09 1.09'
        'a1 1 0 1 1-1.41-1.41l1.09-1.09a4 4 0 0 1 5.66 5.66l-3.59 3.59'
        'a4 4 0 0 1-5.66 0 1 1 0 0 1 1.41-1.43z"/>'
        '<path d="M13.41 10.59a2 2 0 0 0-2.82 0L7 14.18A2 2 0 1 0 9.82 17l1.09-1.09'
        'a1 1 0 1 1 1.41 1.41l-1.09 1.09a4 4 0 1 1-5.66-5.66l3.59-3.59'
        'a4 4 0 0 1 5.66 0 1 1 0 0 1-1.41 1.43z"/>'
        '</svg>'
        '</a>\n'
    )


# =============================================================================
# Tags
# =============================================================================

def get_tags(v):
    tags = []

    if "extern" in v:
        tags.append(str(v["extern"]))

    for attr in v.get("attrs", []):
        if attr not in tags:
            tags.append(attr)

    return tags


def get_class_tags(v):
    tags = get_tags(v)

    if v.get("type") == "extension" and "extend" not in tags:
        tags.insert(0, "extend")

    if v.get("type") == "type" and "tuple" not in tags:
        tags.insert(0, "tuple")

    return tags


def write_tags(tags, f):
    if not tags:
        return

    f.write('<div class="api-tags">\n')

    for tag in tags:
        tooltip = TAG_TOOLTIPS.get(tag, tag)

        f.write(
            '<span '
            'class="api-tag" '
            'tabindex="0" '
            f'data-tooltip="{escape(tooltip)}">'
            f'@{escape(tag)}'
            '</span>\n'
        )

    f.write("</div>\n")


# =============================================================================
# Module representation
# =============================================================================

class ModuleInfo:
    def __init__(
        self,
        module_id,
        source_rel,
        module_name,
        is_init,
    ):
        self.module_id = module_id
        self.source_rel = source_rel
        self.module_name = module_name
        self.is_init = is_init


actual_modules = {}


for module_id, value in j.items():
    if value.get("kind") != "module":
        continue

    source_rel = relative_to_any_root(
        value["path"]
    )

    if not source_rel.endswith(".codon"):
        continue

    without_ext = source_rel[:-6]
    parts = without_ext.split("/")

    is_init = parts[-1] == "__init__"

    if is_init:
        module_parts = parts[:-1]
    else:
        module_parts = parts

    module_name = ".".join(module_parts)

    info = ModuleInfo(
        module_id=module_id,
        source_rel=source_rel,
        module_name=module_name,
        is_init=is_init,
    )

    existing = actual_modules.get(module_name)

    if existing is None or is_init:
        actual_modules[module_name] = info


# =============================================================================
# Logical package tree
# =============================================================================

all_page_names = set(actual_modules)

for module_name in list(actual_modules):
    if not module_name:
        continue

    parts = module_name.split(".")

    for i in range(1, len(parts)):
        all_page_names.add(
            ".".join(parts[:i])
        )


children_by_page = collections.defaultdict(set)

for name in all_page_names:
    if not name:
        continue

    parts = name.split(".")

    if len(parts) == 1:
        parent = ""
        child = parts[0]
    else:
        parent = ".".join(parts[:-1])
        child = parts[-1]

    children_by_page[parent].add(child)


package_names = {
    name
    for name in all_page_names
    if children_by_page.get(name)
}

for name, info in actual_modules.items():
    if info.is_init:
        package_names.add(name)


def output_path_for_page(name):
    """Return the output path for a module page.

    Module pages live under the top-level ``modules`` namespace so the API
    reference has symmetric ``Modules`` and ``Classes`` sections.
    """
    if not name:
        return os.path.join(
            out_path,
            "index.md",
        )

    parts = name.split(".")

    if name in package_names:
        return os.path.join(
            out_path,
            "modules",
            *parts,
            "index.md",
        )

    return os.path.join(
        out_path,
        "modules",
        *parts[:-1],
        parts[-1] + ".md",
    )


# =============================================================================
# Canonical class model
# =============================================================================

class MemberContribution:
    def __init__(
        self,
        value,
        module_name,
        source_rel,
        is_extension,
    ):
        self.value = value
        self.module_name = module_name
        self.source_rel = source_rel
        self.is_extension = is_extension


class ExtensionContribution:
    def __init__(
        self,
        value,
        module_name,
        source_rel,
    ):
        self.value = value
        self.module_name = module_name
        self.source_rel = source_rel


class ClassInfo:
    def __init__(
        self,
        class_id,
        value,
        module_name,
        source_rel,
    ):
        self.class_id = class_id
        self.value = value
        self.name = value.get("name", "")
        self.module_name = module_name
        self.source_rel = source_rel
        self.extensions = []
        self.members = []
        self.display_module_name = module_name

    @property
    def qualified_name(self):
        if self.module_name:
            return f"{self.module_name}.{self.name}"
        return self.name


# Map every direct module child back to the module that owns it. This is also
# useful for finding the defining module of an @extend block's parent class.
module_for_object = {}

for module_name, info in actual_modules.items():
    module = j[info.module_id]

    for child_id in module.get("children", []):
        module_for_object[str(child_id)] = module_name


classes_by_id = {}

# First pass: register real class definitions. Extensions are deliberately
# excluded; they are attached to their canonical parent in the second pass.
for module_name, info in actual_modules.items():
    module = j[info.module_id]

    for child_id in module.get("children", []):
        key = str(child_id)

        if key not in j:
            continue

        value = j[key]

        if (
            value.get("kind") != "class"
            or value.get("type") == "extension"
        ):
            continue

        classes_by_id[key] = ClassInfo(
            class_id=key,
            value=value,
            module_name=module_name,
            source_rel=info.source_rel,
        )


# Second pass: attach every extension to the canonical class referenced by its
# parent field. We use the JSON object ID, never just the class name, so classes
# with the same name in different modules cannot be accidentally merged.
for module_name, info in actual_modules.items():
    module = j[info.module_id]

    for child_id in module.get("children", []):
        key = str(child_id)

        if key not in j:
            continue

        value = j[key]

        if (
            value.get("kind") != "class"
            or value.get("type") != "extension"
        ):
            continue

        parent_id = value.get("parent")

        if parent_id is None:
            print(
                f"warning: extension class in '{module_name}' has no parent",
                file=sys.stderr,
            )
            continue

        parent_id = str(parent_id)
        class_info = classes_by_id.get(parent_id)

        # Be tolerant of a parent class that was not encountered as a direct
        # child during the first pass, as long as the JSON still contains it.
        if class_info is None and parent_id in j:
            parent = j[parent_id]

            if (
                parent.get("kind") == "class"
                and parent.get("type") != "extension"
            ):
                parent_module = module_for_object.get(parent_id, "")
                parent_info = actual_modules.get(parent_module)

                class_info = ClassInfo(
                    class_id=parent_id,
                    value=parent,
                    module_name=parent_module,
                    source_rel=(
                        parent_info.source_rel
                        if parent_info is not None
                        else None
                    ),
                )
                classes_by_id[parent_id] = class_info

        if class_info is None:
            print(
                "warning: extension class "
                f"'{value.get('name', '?')}' in '{module_name}' "
                f"has unknown parent '{parent_id}'",
                file=sys.stderr,
            )
            continue

        class_info.extensions.append(
            ExtensionContribution(
                value=value,
                module_name=module_name,
                source_rel=info.source_rel,
            )
        )


# Aggregate members from the canonical definition and all extension blocks.
def add_class_members(
    class_info,
    value,
    module_name,
    source_rel,
    is_extension,
):
    for member_id in value.get("members", []):
        key = str(member_id)

        if key not in j:
            continue

        member = j[key]

        if member.get("kind") != "function":
            continue

        if (
            not is_public(member)
            and not is_magic_method(member)
        ):
            continue

        class_info.members.append(
            MemberContribution(
                value=member,
                module_name=module_name,
                source_rel=source_rel,
                is_extension=is_extension,
            )
        )


for class_info in classes_by_id.values():
    add_class_members(
        class_info,
        class_info.value,
        class_info.module_name,
        class_info.source_rel,
        False,
    )

    for extension in class_info.extensions:
        add_class_members(
            class_info,
            extension.value,
            extension.module_name,
            extension.source_rel,
            True,
        )


# =============================================================================
# Merge internal implementation classes into public-facing classes
# =============================================================================

def is_internal_class(class_info):
    """Return whether a class is defined in Codon's internal namespace."""
    return (
        class_info.module_name == "internal"
        or class_info.module_name.startswith("internal.")
    )


def member_contribution_key(contribution):
    """
    Return the user-visible identity of a class member contribution.

    The documentation JSON can expose the same method through both a
    top-level/public type and its internal implementation type. Those entries
    have different JSON IDs, so identity by ID is insufficient; the rendered
    name + signature is the stable API identity we care about here.
    """
    value = contribution.value
    return (
        value.get("name", ""),
        parse_fn(value),
    )


def merge_class_info(target, source):
    """
    Merge an implementation-side class into its public-facing counterpart.

    Codon's documentation JSON can contain both a public class (often in the
    top-level namespace) and a separate implementation definition under an
    internal.* module. They represent the same user-facing type and should
    therefore produce one consolidated class page.

    Duplicate members are collapsed by rendered API signature. When the
    public/top-level copy has no module provenance but the internal copy does,
    prefer the internal contribution so the consolidated page can link back to
    the actual defining module.
    """

    # Preserve extension blocks without adding the exact same extension twice.
    existing_extensions = {
        (
            extension.module_name,
            extension.source_rel,
            id(extension.value),
        )
        for extension in target.extensions
    }

    for extension in source.extensions:
        key = (
            extension.module_name,
            extension.source_rel,
            id(extension.value),
        )
        if key not in existing_extensions:
            target.extensions.append(extension)
            existing_extensions.add(key)

    # Merge members by their visible API identity rather than JSON ID. This is
    # what removes pairs such as top-level Set.resize and
    # internal.types.collections.set.Set.resize from the canonical class page.
    member_index = {
        member_contribution_key(contribution): i
        for i, contribution in enumerate(target.members)
    }

    for contribution in source.members:
        key = member_contribution_key(contribution)
        existing_index = member_index.get(key)

        if existing_index is None:
            member_index[key] = len(target.members)
            target.members.append(contribution)
            continue

        existing = target.members[existing_index]

        # A top-level type frequently has module_name == "", which previously
        # rendered as "Defined in stdlib" with no link. The internal duplicate
        # has the useful source module, so use it as the canonical provenance.
        if (
            not existing.module_name
            and contribution.module_name
        ):
            target.members[existing_index] = contribution


# Group real classes by unqualified name.
classes_by_unqualified_name = collections.defaultdict(list)

for class_info in classes_by_id.values():
    if is_public(class_info.value):
        classes_by_unqualified_name[
            class_info.name
        ].append(class_info)


# Map suppressed/alias class IDs to the ClassInfo that owns the single
# consolidated class page. This is important not only for the catalog, but for
# module-level class cards, @references and extension-parent lookups as well.
canonical_class_by_id = {
    class_id: class_info
    for class_id, class_info in classes_by_id.items()
}

suppressed_class_ids = set()


def suppress_into(source, target):
    """Merge source into target and make target the canonical class."""
    if source is target:
        return

    merge_class_info(target, source)
    suppressed_class_ids.add(source.class_id)
    canonical_class_by_id[source.class_id] = target


for name, candidates in classes_by_unqualified_name.items():
    if len(candidates) < 2:
        continue

    top_level_candidates = [
        class_info
        for class_info in candidates
        if not class_info.module_name
    ]

    internal_candidates = [
        class_info
        for class_info in candidates
        if is_internal_class(class_info)
    ]

    public_module_candidates = [
        class_info
        for class_info in candidates
        if class_info.module_name
        and not is_internal_class(class_info)
    ]

    # Case 1: a top-level/re-exported type and exactly one real public module
    # definition share a name. Prefer the module definition as canonical.
    #
    # This is the ndarray pattern:
    #
    #   ndarray                         (top-level exposure / re-export)
    #   numpy.ndarray.ndarray.ndarray   (actual public definition)
    #
    # Keeping the module-qualified definition gives the class page useful
    # provenance and prevents a second link-less /classes/ndarray/ page.
    if (
        len(top_level_candidates) == 1
        and len(public_module_candidates) == 1
    ):
        canonical = public_module_candidates[0]
        top_level = top_level_candidates[0]

        print(
            "info: consolidating class alias "
            f"'{top_level.qualified_name}' -> '{canonical.qualified_name}'",
            file=sys.stderr,
        )
        suppress_into(top_level, canonical)

        # Internal representations, if any, are implementation details of the
        # same public class and can contribute members/extensions as well.
        for internal_class in internal_candidates:
            suppress_into(internal_class, canonical)

        continue

    # Case 2: a top-level built-in-style type has one or more internal.*
    # implementation definitions and no separate public module definition.
    # Keep the top-level identity/URL, but merge the internal implementation
    # into it so methods have real module provenance. Preserve the internal
    # module name as display-only catalog metadata.
    if (
        len(top_level_candidates) == 1
        and not public_module_candidates
        and internal_candidates
    ):
        canonical = top_level_candidates[0]

        for internal_class in internal_candidates:
            print(
                "info: consolidating internal class "
                f"'{internal_class.qualified_name}' -> '{canonical.qualified_name}'",
                file=sys.stderr,
            )
            suppress_into(internal_class, canonical)

        display_source = min(
            internal_candidates,
            key=lambda c: (
                c.module_name.count("."),
                c.module_name,
            ),
        )
        canonical.display_module_name = display_source.module_name
        continue

    # Case 3: there is exactly one non-internal public class and one or more
    # internal implementations, but no top-level alias. Fold only the internal
    # definitions into the public class. This retains legitimate same-named
    # classes in unrelated public modules.
    non_internal_candidates = [
        class_info
        for class_info in candidates
        if not is_internal_class(class_info)
    ]

    if (
        internal_candidates
        and len(non_internal_candidates) == 1
    ):
        canonical = non_internal_candidates[0]

        for internal_class in internal_candidates:
            print(
                "info: consolidating internal class "
                f"'{internal_class.qualified_name}' -> '{canonical.qualified_name}'",
                file=sys.stderr,
            )
            suppress_into(internal_class, canonical)


public_classes = [
    class_info
    for class_info in classes_by_id.values()
    if (
        is_public(class_info.value)
        and class_info.class_id not in suppressed_class_ids
    )
]

public_classes.sort(
    key=lambda class_info: (
        class_info.name.lower(),
        class_info.qualified_name.lower(),
    )
)


class_name_counts = collections.Counter(
    class_info.name
    for class_info in public_classes
)


def class_url_parts(class_info):
    parts = ["classes"]

    if class_info.module_name:
        parts.extend(class_info.module_name.split("."))

    parts.append(class_info.name)
    return parts


def output_path_for_class(class_info):
    return os.path.join(
        out_path,
        *class_url_parts(class_info),
        "index.md",
    )


def class_info_for_value(value):
    """Return the consolidated canonical ClassInfo for a class JSON object."""
    if value.get("kind") != "class":
        return None

    if value.get("type") == "extension":
        parent_id = value.get("parent")

        if parent_id is None:
            return None

        parent_id = str(parent_id)
        return canonical_class_by_id.get(
            parent_id,
            classes_by_id.get(parent_id),
        )

    # Find the ClassInfo that owns this exact JSON value, then canonicalize it
    # in case it was a top-level/internal duplicate that was folded into a
    # different class page.
    for class_id, class_info in classes_by_id.items():
        if class_info.value is value:
            return canonical_class_by_id.get(
                class_id,
                class_info,
            )

    return None


# =============================================================================
# API reference index
# =============================================================================

class ApiReference:
    def __init__(
        self,
        name,
        module_name,
        url_parts,
        anchor=None,
        kind=None,
    ):
        self.name = name
        self.module_name = module_name
        self.url_parts = list(url_parts)
        self.anchor = anchor
        self.kind = kind


api_references = {}
api_aliases = {}


def add_api_reference(
    name,
    module_name,
    url_parts,
    anchor=None,
    kind=None,
):
    if not name:
        return None

    reference = ApiReference(
        name=name,
        module_name=module_name,
        url_parts=url_parts,
        anchor=anchor,
        kind=kind,
    )

    api_references[name] = reference
    return reference


def add_api_alias(name, reference):
    if name and reference is not None:
        api_aliases[name] = reference


def build_api_reference_index():
    """
    Build the reference index before rendering documentation.

    Module-level functions and variables point to module pages. Classes and
    class members point to their consolidated class pages, including members
    contributed by @extend blocks in other modules.
    """

    # Modules and logical packages.
    for module_name in all_page_names:
        if module_name:
            add_api_reference(
                module_name,
                module_name,
                ["modules", *module_name.split(".")],
                kind="module",
            )

    # Module-level functions and variables.
    for module_name, info in actual_modules.items():
        module = j[info.module_id]

        for child_id in module.get("children", []):
            key = str(child_id)

            if key not in j:
                continue

            value = j[key]

            if not is_public(value):
                continue

            kind = value.get("kind")
            name = value.get("name")

            if not name:
                continue

            qualified_name = (
                f"{module_name}.{name}"
                if module_name
                else name
            )

            if kind == "function":
                add_api_reference(
                    qualified_name,
                    module_name,
                    (["modules", *module_name.split(".")] if module_name else ["modules"]),
                    anchor=function_anchor(name),
                    kind="function",
                )

            elif kind == "variable":
                add_api_reference(
                    qualified_name,
                    module_name,
                    (["modules", *module_name.split(".")] if module_name else ["modules"]),
                    anchor=variable_anchor(name),
                    kind="variable",
                )

    # Canonical classes and every member contributed to them.
    for class_info in public_classes:
        class_reference = add_api_reference(
            class_info.qualified_name,
            class_info.module_name,
            class_url_parts(class_info),
            kind="class",
        )

        # Preserve every suppressed/re-exported qualified class name as an
        # alias of the consolidated page. For example, both @ndarray and the
        # module-qualified ndarray definition resolve to the same class page.
        for source_id, canonical in canonical_class_by_id.items():
            if canonical is not class_info or source_id == class_info.class_id:
                continue

            source = classes_by_id.get(source_id)
            if source is None:
                continue

            add_api_alias(
                source.qualified_name,
                class_reference,
            )

        # An unambiguous class can also be referenced by its simple name.
        # This is particularly useful for built-in-style APIs such as @str.
        if class_name_counts[class_info.name] == 1:
            add_api_alias(
                class_info.name,
                class_reference,
            )

        by_name = collections.defaultdict(list)

        for contribution in class_info.members:
            by_name[contribution.value["name"]].append(contribution)

        owner_anchor = class_anchor(class_info.name)

        for member_name, contributions in by_name.items():
            kind = (
                "property"
                if any(
                    "property" in contribution.value.get("attrs", [])
                    for contribution in contributions
                )
                else "method"
            )

            reference = add_api_reference(
                f"{class_info.qualified_name}.{member_name}",
                class_info.module_name,
                class_url_parts(class_info),
                anchor=member_anchor(owner_anchor, member_name),
                kind=kind,
            )

            if class_name_counts[class_info.name] == 1:
                add_api_alias(
                    f"{class_info.name}.{member_name}",
                    reference,
                )

            # Suppressed/re-exported class identities should resolve members
            # to this same consolidated member group too.
            for source_id, canonical in canonical_class_by_id.items():
                if canonical is not class_info or source_id == class_info.class_id:
                    continue

                source = classes_by_id.get(source_id)
                if source is None:
                    continue

                add_api_alias(
                    f"{source.qualified_name}.{member_name}",
                    reference,
                )

            # An extension implementation can use same-module shorthand even
            # though the canonical class lives in another module.
            for contribution in contributions:
                alias = (
                    f"{contribution.module_name}.{class_info.name}.{member_name}"
                    if contribution.module_name
                    else f"{class_info.name}.{member_name}"
                )
                add_api_alias(alias, reference)

        # Likewise make the extension module's Class name resolve to the
        # canonical class page from docstrings written in that module.
        for extension in class_info.extensions:
            alias = (
                f"{extension.module_name}.{class_info.name}"
                if extension.module_name
                else class_info.name
            )
            add_api_alias(alias, class_reference)


def api_reference_url(reference):
    """Return a URL relative to the API Reference root."""
    if reference.url_parts:
        href = "/".join(reference.url_parts) + "/"
    else:
        href = "./"

    if reference.anchor:
        href += f"#{reference.anchor}"

    return href


def api_reference_short_name(reference):
    return reference.name.rsplit(".", 1)[-1]


def write_search_index():
    """Write the client-side API symbol search index."""
    entries = []

    # Only canonical references are indexed. Aliases exist for link resolution
    # but would create duplicate search results.
    for reference in api_references.values():
        entries.append({
            "name": api_reference_short_name(reference),
            "qualified": reference.name,
            "kind": reference.kind,
            "url": api_reference_url(reference),
        })

    entries.sort(
        key=lambda entry: (
            entry["qualified"].lower(),
            entry["kind"] or "",
        )
    )

    path = os.path.join(
        out_path,
        "api-search-index.json",
    )

    with open(path, "w") as f:
        json.dump(
            entries,
            f,
            ensure_ascii=False,
            separators=(",", ":"),
        )


build_api_reference_index()


# =============================================================================
# API reference resolution
# =============================================================================

API_REFERENCE_RE = re.compile(
    r"""
    \(
        @
        (?P<target>[A-Za-z_][A-Za-z0-9_.]*)
    \)
    """,
    re.VERBOSE,
)


def module_url_parts(module_name):
    """Return URL components for a module page."""
    if not module_name:
        return ["modules"]
    return ["modules", *module_name.split(".")]


def relative_url(current_parts, target_parts):
    """Compute a pretty relative URL between two generated API pages."""
    current = list(current_parts)
    target = list(target_parts)

    common = 0

    while (
        common < len(current)
        and common < len(target)
        and current[common] == target[common]
    ):
        common += 1

    up = [".."] * (len(current) - common)
    down = target[common:]
    parts = up + down

    if not parts:
        return ""

    return "/".join(parts) + "/"


def find_api_reference(target, current_module):
    """
    Resolve an @reference. Fully-qualified names are tried first, followed by
    aliases and then names relative to the current implementation module.
    """
    reference = api_references.get(target)

    if reference is not None:
        return reference

    reference = api_aliases.get(target)

    if reference is not None:
        return reference

    if current_module:
        local_target = f"{current_module}.{target}"

        reference = api_references.get(local_target)

        if reference is not None:
            return reference

        reference = api_aliases.get(local_target)

        if reference is not None:
            return reference

    return None


def api_reference_href(
    reference,
    current_module,
    current_url_parts=None,
):
    """Generate an href for a resolved API reference."""
    if current_url_parts is None:
        current_url_parts = module_url_parts(current_module)

    if list(current_url_parts) == reference.url_parts:
        if reference.anchor:
            return f"#{reference.anchor}"
        return "./"

    href = relative_url(
        current_url_parts,
        reference.url_parts,
    )

    if reference.anchor:
        href += f"#{reference.anchor}"

    return href


def resolve_api_links(
    doc,
    current_module,
    current_url_parts=None,
):
    """
    Resolve @ references used as Markdown link destinations.

    References inside fenced or inline code are intentionally left untouched.
    Same-module shorthand remains supported, including from an @extend block.
    """

    protected = []

    def protect(match):
        protected.append(match.group(0))
        return f"\x00CODON_CODE_{len(protected) - 1}\x00"

    # This is deliberately modest rather than a full Markdown parser: it covers
    # the common fenced-code and inline-code forms used in stdlib docstrings.
    code_re = re.compile(
        r"```.*?```|~~~.*?~~~|`[^`\n]*`",
        re.DOTALL,
    )
    doc = code_re.sub(protect, doc)

    def replace(match):
        target = match.group("target")
        reference = find_api_reference(
            target,
            current_module,
        )

        if reference is None:
            print(
                "warning: unresolved API reference "
                f"'@{target}' in module '{current_module}'",
                file=sys.stderr,
            )
            return match.group(0)

        href = api_reference_href(
            reference,
            current_module,
            current_url_parts,
        )

        return f"({href})"

    doc = API_REFERENCE_RE.sub(replace, doc)

    for i, value in enumerate(protected):
        doc = doc.replace(f"\x00CODON_CODE_{i}\x00", value)

    return doc


# =============================================================================
# Generic output
# =============================================================================

def parse_docstr(s):
    """Normalize a docstring while preserving its Markdown."""
    if not s:
        return ""

    return textwrap.dedent(s).strip()


DOC_MARKDOWN_EXTENSIONS = [
    # Core Markdown already provides paragraphs, headings, emphasis, lists,
    # blockquotes and links. These add the common extras we want in docstrings.
    "admonition",
    "attr_list",
    "tables",
    "pymdownx.highlight",
    "pymdownx.inlinehilite",
    "pymdownx.superfences",
]


def render_docstr(
    s,
    current_module,
    current_url_parts=None,
):
    """Render a Codon docstring from Markdown to HTML."""
    s = parse_docstr(s)

    if not s:
        return ""

    s = resolve_api_links(
        s,
        current_module,
        current_url_parts,
    )

    return markdown.markdown(
        s,
        extensions=DOC_MARKDOWN_EXTENSIONS,
        output_format="html5",
    )


def write_docstr(
    v,
    f,
    current_module,
    current_url_parts=None,
):
    doc = v.get("doc")

    if not doc:
        return

    # Inline-LLVM docstrings contain LLVM source rather than Markdown.
    if "llvm" in v.get("attrs", []):
        f.write('<div class="api-doc">\n')
        f.write('<pre><code class="language-llvm">')
        f.write(html.escape(parse_docstr(doc)))
        f.write("</code></pre>\n")
        f.write("</div>\n")
        return

    rendered = render_docstr(
        doc,
        current_module,
        current_url_parts,
    )

    if rendered:
        f.write('<div class="api-doc">\n')
        f.write(rendered)
        f.write("\n</div>\n")


def write_frontmatter(title, f):
    title = title.replace('"', '\\"')

    f.write("---\n")
    f.write(f'title: "{title}"\n')
    f.write("---\n\n")


def write_signature(signature, f):
    f.write(
        '<div class="api-signature" markdown="1">\n\n'
    )

    f.write(
        "``` { .python .no-copy }\n"
    )

    f.write(signature)
    f.write("\n```\n\n")
    f.write("</div>\n\n")


def write_section_heading(
    title,
    count,
    f,
):
    f.write(
        '<div class="api-section-heading">\n'
    )

    f.write(
        f'<span class="api-section-title">'
        f'{escape(title)}'
        f'</span>\n'
    )

    if count is not None:
        f.write(
            f'<span class="api-section-count">'
            f'{count}'
            f'</span>\n'
        )

    f.write("</div>\n")


# =============================================================================
# Breadcrumbs
# =============================================================================

def write_breadcrumbs(
    module_name,
    f,
):
    if not module_name:
        return

    parts = module_name.split(".")
    current_parts = module_url_parts(module_name)

    f.write(
        '<nav class="api-breadcrumbs" '
        'aria-label="Breadcrumb">\n'
    )

    root_href = relative_url(current_parts, [])
    f.write(
        f'<a href="{escape(root_href)}" '
        f'class="api-breadcrumb-link">'
        'API Reference'
        '</a>\n'
    )

    f.write(
        '<span class="api-breadcrumb-separator" '
        'aria-hidden="true">/</span>\n'
    )

    modules_href = relative_url(current_parts, ["modules"])
    f.write(
        f'<a href="{escape(modules_href)}" '
        f'class="api-breadcrumb-link">Modules</a>\n'
    )

    for i, part in enumerate(parts):
        f.write(
            '<span class="api-breadcrumb-separator" '
            'aria-hidden="true">/</span>\n'
        )

        last = i == len(parts) - 1

        if last:
            f.write(
                '<span class="api-breadcrumb-current">'
                f'{escape(part)}'
                '</span>\n'
            )
        else:
            target_module = ".".join(parts[:i + 1])
            href = relative_url(
                current_parts,
                module_url_parts(target_module),
            )
            f.write(
                f'<a href="{escape(href)}" '
                f'class="api-breadcrumb-link">'
                f'{escape(part)}'
                '</a>\n'
            )

    f.write("</nav>\n\n")


# =============================================================================
# Page header
# =============================================================================

def write_page_header(
    module_name,
    source_rel,
    is_package,
    f,
):
    write_breadcrumbs(
        module_name,
        f,
    )

    source_url = (
        f"{GITHUB_STDLIB_URL}/{source_rel}"
        if source_rel
        else None
    )

    f.write(
        '<header class="api-page-header">\n'
    )

    f.write(
        f'<h1 class="api-page-title">'
        f'{escape(module_name)}'
        f'</h1>\n'
    )

    f.write(
        '<div class="api-page-meta">\n'
    )

    f.write(
        '<span class="api-page-kind">'
        f'{"Standard library package" if is_package else "Standard library module"}'
        '</span>\n'
    )

    if source_url:
        f.write(
            f'<a class="api-source-link" '
            f'href="{escape(source_url)}" '
            f'target="_blank" '
            f'rel="noopener noreferrer">'
        )

        f.write(
            '<span class="api-source-icon" '
            'aria-hidden="true">'
        )

        f.write(GITHUB_ICON)

        f.write(
            '</span>'
            '<span class="api-source-text">'
            'View source'
            '</span>'
            '</a>\n'
        )

    f.write("</div>\n")
    f.write("</header>\n\n")


# =============================================================================
# API cards
# =============================================================================

def write_card_start(
    name,
    kind,
    tags,
    f,
    anchor,
):
    f.write(
        f'<article '
        f'class="api-card api-card-{escape(kind)}" '
        f'id="{escape(anchor)}">\n'
    )

    f.write(
        '<header class="api-card-header">\n'
    )

    f.write(
        '<div class="api-card-title-group">\n'
    )

    f.write(
        f'<span '
        f'class="api-kind api-kind-{escape(kind)}">'
        f'{escape(kind)}'
        f'</span>\n'
    )

    f.write(
        f'<span class="api-card-name">'
        f'{escape(name)}'
        f'</span>\n'
    )

    write_permalink(
        anchor,
        f,
        f"{kind} {name}",
    )

    f.write("</div>\n")

    write_tags(
        tags,
        f,
    )

    f.write("</header>\n")

    f.write(
        '<div class="api-card-body">\n\n'
    )


def write_card_end(f):
    f.write("</div>\n")
    f.write("</article>\n\n")


# =============================================================================
# Functions and variables
# =============================================================================

def write_function_variants(
    values,
    f,
    current_module,
):
    """Render all overloads of one module-level function in one API card."""
    if not values:
        return

    name = values[0]["name"]
    anchor = function_anchor(name)

    shared_tags = common_tags(values)

    write_card_start(
        name,
        "function",
        shared_tags,
        f,
        anchor,
    )

    for value in values:
        if len(values) > 1:
            f.write('<div class="api-function-overload">\n')

        variant_tags = overload_specific_tags(value, shared_tags)
        if variant_tags:
            f.write('<div class="api-overload-tags">\n')
            write_tags(variant_tags, f)
            f.write('</div>\n')

        write_signature(
            f'{name}{parse_fn(value)}',
            f,
        )

        write_docstr(
            value,
            f,
            current_module,
        )

        if len(values) > 1:
            f.write('</div>\n')

    write_card_end(f)


def write_function(
    v,
    f,
    current_module,
):
    # Retained for callers that render a single standalone function.
    write_function_variants([v], f, current_module)


def write_variable(
    v,
    f,
    current_module,
):
    declaration = v["name"]

    if v.get("type"):
        declaration += (
            ": " + parse_type(v["type"])
        )

    if "value" in v:
        declaration += (
            " = " + str(v["value"])
        )

    anchor = variable_anchor(
        v["name"]
    )

    write_card_start(
        v["name"],
        "variable",
        get_tags(v),
        f,
        anchor,
    )

    write_signature(
        declaration,
        f,
    )

    write_docstr(
        v,
        f,
        current_module,
    )

    write_card_end(f)


# =============================================================================
# Fields
# =============================================================================

def write_fields(fields, f):
    if not fields:
        return

    f.write(
        '<section class="api-card-section">\n'
    )

    write_section_heading(
        "Fields",
        len(fields),
        f,
    )

    f.write(
        '<div class="api-fields">\n'
    )

    for field in fields:
        field_type = (
            parse_type(field["type"])
            if field.get("type")
            else "Any"
        )

        f.write(
            '<div class="api-field">\n'
        )

        f.write(
            f'<code class="api-field-name">'
            f'{escape(field["name"])}'
            f'</code>\n'
        )

        f.write(
            f'<code class="api-field-type">'
            f'{escape(field_type)}'
            f'</code>\n'
        )

        f.write("</div>\n")

    f.write("</div>\n")
    f.write("</section>\n")


# =============================================================================
# Class members
# =============================================================================

def visible_members(value):
    members = []

    for member_id in value.get("members", []):
        key = str(member_id)

        if key not in j:
            continue

        member = j[key]

        if member.get("kind") != "function":
            continue

        if (
            not is_public(member)
            and not is_magic_method(member)
        ):
            continue

        members.append(member)

    return members


def group_member_values(members):
    groups = collections.OrderedDict()

    for member in members:
        groups.setdefault(member["name"], []).append(member)

    return list(groups.values())


def group_member_contributions(contributions):
    groups = collections.OrderedDict()

    for contribution in contributions:
        groups.setdefault(
            contribution.value["name"],
            [],
        ).append(contribution)

    return list(groups.values())


def common_tags(values):
    """Return tags that are present on every overload, preserving first-overload order."""
    if not values:
        return []

    first = get_tags(values[0])
    if len(values) == 1:
        return first

    remaining = [set(get_tags(value)) for value in values[1:]]
    return [tag for tag in first if all(tag in tags for tags in remaining)]


def overload_specific_tags(value, shared_tags):
    """Return tags belonging specifically to one overload."""
    shared = set(shared_tags)
    return [tag for tag in get_tags(value) if tag not in shared]


def write_member_variants(
    values,
    owner_anchor,
    f,
    current_module,
    current_url_parts=None,
    contributions=None,
    anchor_override=None,
):
    """
    Render all overloads of one member under one stable anchor.

    If contributions is provided, it must correspond one-to-one with values
    and source-module provenance is displayed for the consolidated class page.
    """
    if not values:
        return

    name = values[0]["name"]
    anchor = anchor_override or member_anchor(owner_anchor, name)

    f.write(
        f'<div class="api-member" id="{escape(anchor)}">\n\n'
    )

    f.write('<div class="api-member-header">\n')
    f.write('<div class="api-member-title-group">\n')
    f.write(
        f'<span class="api-member-name">{escape(name)}</span>\n'
    )

    write_permalink(anchor, f, name)
    f.write("</div>\n")

    shared_tags = common_tags(values)
    write_tags(shared_tags, f)
    f.write("</div>\n\n")

    for i, value in enumerate(values):
        if len(values) > 1:
            f.write('<div class="api-member-overload">\n')

        variant_tags = overload_specific_tags(value, shared_tags)
        if variant_tags:
            f.write('<div class="api-overload-tags">\n')
            write_tags(variant_tags, f)
            f.write('</div>\n')

        write_signature(
            f"{name}{parse_fn(value)}",
            f,
        )

        if contributions is not None:
            contribution = contributions[i]
            source_parts = module_url_parts(
                contribution.module_name
            )
            href = relative_url(
                current_url_parts or [],
                source_parts,
            )

            f.write('<div class="api-member-source">')
            f.write(
                'Added by '
                if contribution.is_extension
                else 'Defined in '
            )

            if contribution.module_name:
                f.write(
                    f'<a href="{escape(href)}">'
                    f'<code>{escape(contribution.module_name)}</code>'
                    '</a>'
                )
            else:
                f.write('<code>stdlib</code>')

            f.write("</div>\n")

        doc_module = (
            contributions[i].module_name
            if contributions is not None
            else current_module
        )

        write_docstr(
            value,
            f,
            doc_module,
            current_url_parts,
        )

        if len(values) > 1:
            f.write("</div>\n")

    f.write("</div>\n")


def write_member_group(
    title,
    members,
    owner_anchor,
    f,
    current_module,
    current_url_parts=None,
    group_overloads=True,
):
    if not members:
        return

    f.write('<section class="api-card-section">\n')

    if group_overloads:
        groups = group_member_values(members)
        write_section_heading(title, len(groups), f)
        f.write('<div class="api-members">\n')

        for values in groups:
            write_member_variants(
                values,
                owner_anchor,
                f,
                current_module,
                current_url_parts,
            )
    else:
        # Constructors are intentionally rendered one-by-one. Multiple
        # __init__ or __new__ definitions are distinct constructor overloads
        # and should each be visible/count separately.
        write_section_heading(title, len(members), f)
        f.write('<div class="api-members">\n')

        seen = collections.defaultdict(int)

        for member in members:
            name = member["name"]
            seen[name] += 1
            base_anchor = member_anchor(owner_anchor, name)
            anchor = (
                base_anchor
                if seen[name] == 1
                else f"{base_anchor}-overload-{seen[name]}"
            )

            write_member_variants(
                [member],
                owner_anchor,
                f,
                current_module,
                current_url_parts,
                anchor_override=anchor,
            )

    f.write("</div>\n")
    f.write("</section>\n")


def write_contribution_group(
    title,
    contributions,
    owner_anchor,
    f,
    class_info,
    group_overloads=True,
):
    if not contributions:
        return

    current_parts = class_url_parts(class_info)
    f.write('<section class="api-card-section">\n')

    if group_overloads:
        groups = group_member_contributions(contributions)
        write_section_heading(title, len(groups), f)
        f.write('<div class="api-members">\n')

        for group in groups:
            write_member_variants(
                [contribution.value for contribution in group],
                owner_anchor,
                f,
                class_info.module_name,
                current_parts,
                contributions=group,
            )
    else:
        write_section_heading(title, len(contributions), f)
        f.write('<div class="api-members">\n')

        seen = collections.defaultdict(int)

        for contribution in contributions:
            name = contribution.value["name"]
            seen[name] += 1
            base_anchor = member_anchor(owner_anchor, name)
            anchor = (
                base_anchor
                if seen[name] == 1
                else f"{base_anchor}-overload-{seen[name]}"
            )

            write_member_variants(
                [contribution.value],
                owner_anchor,
                f,
                class_info.module_name,
                current_parts,
                contributions=[contribution],
                anchor_override=anchor,
            )

    f.write("</div>\n")
    f.write("</section>\n")


# =============================================================================
# Classes
# =============================================================================

def write_class(
    v,
    f,
    current_module,
    display_name=None,
):
    """Render a class or @extend block on its implementation module page."""
    display_name = display_name or v["name"]
    owner_anchor = class_anchor(display_name)

    write_card_start(
        display_name,
        "class",
        get_class_tags(v),
        f,
        owner_anchor,
    )

    write_signature(
        class_signature(v, display_name),
        f,
    )

    class_info = class_info_for_value(v)

    if (
        v.get("type") == "extension"
        and class_info is not None
    ):
        href = relative_url(
            module_url_parts(current_module),
            class_url_parts(class_info),
        )

        f.write('<div class="api-class-canonical-link">\n')
        f.write('Extends ')
        f.write(
            f'<a href="{escape(href)}">'
            f'<code>{escape(class_info.qualified_name)}</code>'
            '</a>'
        )
        f.write(' · ')
        f.write(
            f'<a href="{escape(href)}">View complete class API →</a>\n'
        )
        f.write('</div>\n')

    elif class_info is not None:
        href = relative_url(
            module_url_parts(current_module),
            class_url_parts(class_info),
        )

        f.write('<div class="api-class-canonical-link">\n')
        f.write(
            f'<a href="{escape(href)}">View complete class API →</a>\n'
        )
        f.write('</div>\n')

    write_docstr(
        v,
        f,
        current_module,
    )

    fields = [
        field
        for field in v.get("args", [])
        if field.get("name")
        and not field["name"].startswith("_")
    ]

    write_fields(fields, f)

    members = visible_members(v)

    constructors = [
        member
        for member in members
        if member.get("name") in ("__init__", "__new__")
    ]

    properties = [
        member
        for member in members
        if is_public(member)
        and "property" in member.get("attrs", [])
    ]

    methods = [
        member
        for member in members
        if is_public(member)
        and not is_magic_method(member)
        and member not in properties
    ]

    magic_methods = [
        member
        for member in members
        if is_magic_method(member)
        and member not in constructors
    ]

    write_member_group(
        "Constructors",
        constructors,
        owner_anchor,
        f,
        current_module,
        group_overloads=False,
    )

    write_member_group(
        "Properties",
        properties,
        owner_anchor,
        f,
        current_module,
    )

    write_member_group(
        "Methods",
        methods,
        owner_anchor,
        f,
        current_module,
    )

    write_member_group(
        "Special methods",
        magic_methods,
        owner_anchor,
        f,
        current_module,
    )

    write_card_end(f)


def write_class_breadcrumbs(class_info, f):
    parts = class_info.module_name.split(".") if class_info.module_name else []
    current_parts = class_url_parts(class_info)

    f.write('<nav class="api-breadcrumbs" aria-label="Breadcrumb">\n')

    root_href = relative_url(current_parts, [])
    f.write(
        f'<a href="{escape(root_href)}" class="api-breadcrumb-link">'
        'API Reference</a>\n'
    )

    f.write(
        '<span class="api-breadcrumb-separator" aria-hidden="true">/</span>\n'
    )

    classes_href = relative_url(current_parts, ["classes"])
    f.write(
        f'<a href="{escape(classes_href)}" class="api-breadcrumb-link">'
        'Classes</a>\n'
    )

    for i, part in enumerate(parts):
        f.write(
            '<span class="api-breadcrumb-separator" aria-hidden="true">/</span>\n'
        )

        target_module = ".".join(parts[:i + 1])
        href = relative_url(
            current_parts,
            module_url_parts(target_module),
        )
        f.write(
            f'<a href="{escape(href)}" class="api-breadcrumb-link">'
            f'{escape(part)}</a>\n'
        )

    f.write(
        '<span class="api-breadcrumb-separator" aria-hidden="true">/</span>\n'
    )
    f.write(
        f'<span class="api-breadcrumb-current">{escape(class_info.name)}</span>\n'
    )
    f.write('</nav>\n\n')


def write_class_page_header(class_info, f):
    write_class_breadcrumbs(class_info, f)

    source_url = (
        f"{GITHUB_STDLIB_URL}/{class_info.source_rel}"
        if class_info.source_rel
        else None
    )

    f.write('<header class="api-page-header">\n')
    f.write(
        f'<h1 class="api-page-title">{escape(class_info.name)}</h1>\n'
    )
    f.write('<div class="api-page-meta">\n')
    f.write('<span class="api-page-kind">Standard library class</span>\n')

    if class_info.qualified_name != class_info.name:
        f.write(
            f'<code class="api-page-qualified">'
            f'{escape(class_info.qualified_name)}</code>\n'
        )

    if source_url:
        f.write(
            f'<a class="api-source-link" '
            f'href="{escape(source_url)}" '
            'target="_blank" rel="noopener noreferrer">'
        )
        f.write('<span class="api-source-icon" aria-hidden="true">')
        f.write(GITHUB_ICON)
        f.write(
            '</span><span class="api-source-text">View source</span></a>\n'
        )

    f.write('</div>\n')
    f.write('</header>\n\n')


def consolidated_class_fields(class_info):
    """Collect public fields from every definition folded into a class page.

    A field name represents one logical field. Duplicate definitions can carry
    different amounts of type information (for example ``Literal[?]`` on a
    top-level alias and ``Literal[int]`` on its implementation definition), so
    prefer the most informative version rather than displaying both.
    """
    values = [class_info.value]

    for source_id, canonical in canonical_class_by_id.items():
        if canonical is not class_info or source_id == class_info.class_id:
            continue
        source = classes_by_id.get(source_id)
        if source is not None:
            values.append(source.value)

    # @extend normally contributes methods, but include fields defensively.
    values.extend(extension.value for extension in class_info.extensions)

    def field_type_text(field):
        return parse_type(field.get("type")) if field.get("type") else "Any"

    def field_quality(field):
        """Prefer concrete type information over unknown/placeholder types."""
        text = field_type_text(field)
        return (
            "?" not in text and text != "Any",
            text != "Any",
            -text.count("?"),
            len(text),
        )

    fields_by_name = collections.OrderedDict()

    for value in values:
        for field in value.get("args", []):
            name = field.get("name")
            if not name or name.startswith("_"):
                continue

            existing = fields_by_name.get(name)
            if existing is None or field_quality(field) > field_quality(existing):
                fields_by_name[name] = field

    return list(fields_by_name.values())


def write_consolidated_class(class_info, f):
    value = class_info.value
    owner_anchor = class_anchor(class_info.name)
    current_parts = class_url_parts(class_info)

    write_card_start(
        class_info.name,
        "class",
        get_class_tags(value),
        f,
        owner_anchor,
    )

    write_signature(
        class_signature(value, class_info.name),
        f,
    )

    write_docstr(
        value,
        f,
        class_info.module_name,
        current_parts,
    )

    fields = consolidated_class_fields(class_info)
    write_fields(fields, f)

    constructors = [
        contribution
        for contribution in class_info.members
        if contribution.value.get("name") in ("__init__", "__new__")
    ]

    properties = [
        contribution
        for contribution in class_info.members
        if is_public(contribution.value)
        and "property" in contribution.value.get("attrs", [])
    ]

    methods = [
        contribution
        for contribution in class_info.members
        if is_public(contribution.value)
        and not is_magic_method(contribution.value)
        and "property" not in contribution.value.get("attrs", [])
    ]

    magic_methods = [
        contribution
        for contribution in class_info.members
        if is_magic_method(contribution.value)
        and contribution not in constructors
    ]

    write_contribution_group(
        "Constructors",
        constructors,
        owner_anchor,
        f,
        class_info,
        group_overloads=False,
    )
    write_contribution_group(
        "Properties",
        properties,
        owner_anchor,
        f,
        class_info,
    )
    write_contribution_group(
        "Methods",
        methods,
        owner_anchor,
        f,
        class_info,
    )
    write_contribution_group(
        "Special methods",
        magic_methods,
        owner_anchor,
        f,
        class_info,
    )

    write_card_end(f)


# =============================================================================
# Render contents
# =============================================================================

def render_module_contents(
    info,
    f,
    current_module,
):
    if info is None:
        return

    module = j[info.module_id]

    write_docstr(
        module,
        f,
        current_module,
    )

    children = []

    for child_id in module.get("children", []):
        key = str(child_id)
        if key in j:
            children.append(j[key])

    function_groups = collections.OrderedDict()
    for value in children:
        if is_public(value) and value.get("kind") == "function":
            function_groups.setdefault(value["name"], []).append(value)

    emitted_functions = set()

    for v in children:
        if not is_public(v):
            continue

        kind = v.get("kind")

        if kind == "class":
            display_name = v["name"]
            parent_id = v.get("parent")
            parent_key = str(parent_id) if parent_id is not None else None

            if (
                v.get("type") == "extension"
                and parent_key in j
            ):
                display_name = j[parent_key]["name"]

            write_class(
                v,
                f,
                current_module,
                display_name,
            )

        elif kind == "function":
            name = v["name"]
            if name in emitted_functions:
                continue
            emitted_functions.add(name)
            write_function_variants(
                function_groups[name],
                f,
                current_module,
            )

        elif kind == "variable":
            write_variable(
                v,
                f,
                current_module,
            )


# =============================================================================
# Child modules
# =============================================================================

def write_child_modules(
    page_name,
    f,
    current_parts=None,
):
    children = sorted(
        children_by_page.get(
            page_name,
            [],
        )
    )

    if not children:
        return

    if current_parts is None:
        current_parts = module_url_parts(page_name)

    f.write(
        '<section class="api-module-browser">\n'
    )

    write_section_heading(
        "Modules",
        len(children),
        f,
    )

    f.write(
        '<div class="api-module-grid">\n'
    )

    for child in children:
        child_name = (
            f"{page_name}.{child}"
            if page_name
            else child
        )
        href = relative_url(
            current_parts,
            module_url_parts(child_name),
        )

        f.write(
            f'<a class="api-module-card" '
            f'href="{escape(href)}">\n'
        )

        f.write(
            f'<code class="api-module-card-name">'
            f'{escape(child)}'
            f'</code>\n'
        )

        f.write(
            '<span class="api-module-card-arrow" '
            'aria-hidden="true">→</span>\n'
        )

        f.write("</a>\n")

    f.write("</div>\n")
    f.write("</section>\n")


def write_class_catalog(f, current_parts=None):
    if not public_classes:
        return

    if current_parts is None:
        current_parts = []

    f.write('<section class="api-class-browser">\n')
    write_section_heading("Classes", len(public_classes), f)

    f.write('<div class="api-class-scroll-wrap">\n')
    f.write('<div class="api-class-scroll">\n')
    f.write('<div class="api-module-grid api-class-grid">\n')

    for class_info in public_classes:
        href = relative_url(
            current_parts,
            class_url_parts(class_info),
        )

        f.write(
            f'<a class="api-module-card api-class-card" '
            f'href="{escape(href)}">\n'
        )

        f.write('<span class="api-class-card-main">\n')

        f.write(
            f'<code class="api-module-card-name">'
            f'{escape(class_info.name)}'
            f'</code>\n'
        )

        if class_info.display_module_name:
            f.write(
                f'<span class="api-class-card-module">'
                f'{escape(class_info.display_module_name)}'
                f'</span>\n'
            )

        f.write("</span>\n")

        f.write(
            '<span class="api-module-card-arrow" '
            'aria-hidden="true">→</span>\n'
        )

        f.write("</a>\n")

    f.write('</div>\n')
    f.write('</div>\n')
    f.write(
        '<div class="api-class-scroll-hint" aria-hidden="true">'
        '<span>Scroll for more</span>'
        '<span class="api-class-scroll-hint-arrow">↓</span>'
        '</div>\n'
    )
    f.write('</div>\n')
    f.write('</section>\n')


def write_modules_index(f):
    write_frontmatter("Standard Library Modules", f)

    f.write('<nav class="api-breadcrumbs" aria-label="Breadcrumb">\n')
    f.write('<a href="../" class="api-breadcrumb-link">API Reference</a>\n')
    f.write(
        '<span class="api-breadcrumb-separator" aria-hidden="true">/</span>\n'
    )
    f.write('<span class="api-breadcrumb-current">Modules</span>\n')
    f.write('</nav>\n\n')

    f.write('<header class="api-page-header">\n')
    f.write('<h1 class="api-page-title">Modules</h1>\n')
    f.write(
        '<div class="api-page-description">'
        'Codon standard library modules and packages.'
        '</div>\n'
    )
    f.write('</header>\n\n')

    write_child_modules("", f, ["modules"])


def write_classes_index(f):
    write_frontmatter("Standard Library Classes", f)

    f.write('<nav class="api-breadcrumbs" aria-label="Breadcrumb">\n')
    f.write('<a href="../" class="api-breadcrumb-link">API Reference</a>\n')
    f.write(
        '<span class="api-breadcrumb-separator" aria-hidden="true">/</span>\n'
    )
    f.write('<span class="api-breadcrumb-current">Classes</span>\n')
    f.write('</nav>\n\n')

    f.write('<header class="api-page-header">\n')
    f.write('<h1 class="api-page-title">Classes</h1>\n')
    f.write(
        '<div class="api-page-description">'
        'Complete class APIs, including methods contributed by '
        '<code>@extend</code> blocks across the standard library.'
        '</div>\n'
    )
    f.write('</header>\n\n')

    write_class_catalog(f, ["classes"])


def write_api_search(f):
    f.write(
        '<div class="api-search">\n'
        '  <div class="api-search-input-wrap">\n'
        '    <svg class="api-search-icon" '
        'viewBox="0 0 24 24" aria-hidden="true">\n'
        '      <path d="M9.5 3a6.5 6.5 0 1 0 3.98 11.64'
        'L19.85 21 21 19.85l-6.36-6.37A6.5 6.5 0 0 0 9.5 3zm0 2'
        'a4.5 4.5 0 1 1 0 9 4.5 4.5 0 0 1 0-9z"/>\n'
        '    </svg>\n'
        '    <input '
        'id="api-search-input" '
        'class="api-search-input" '
        'type="search" '
        'placeholder="Search modules, classes, functions, methods..." '
        'autocomplete="off" '
        'spellcheck="false" '
        'role="combobox" '
        'aria-autocomplete="list" '
        'aria-label="Search API reference" '
        'aria-controls="api-search-results" '
        'aria-expanded="false">\n'
        '    <span class="api-search-shortcut" aria-hidden="true">/</span>\n'
        '  </div>\n'
        '  <div '
        'id="api-search-results" '
        'class="api-search-results" '
        'role="listbox" '
        'hidden>\n'
        '  </div>\n'
        '</div>\n\n'
    )


# =============================================================================
# Generate root
# =============================================================================

os.makedirs(
    out_path,
    exist_ok=True,
)

root_path = output_path_for_page("")

with open(root_path, "w") as f:
    write_frontmatter(
        "Standard Library Reference",
        f,
    )

    f.write(
        '<header class="api-page-header api-root-header">\n'
    )
    f.write(
        '<h1 class="api-page-title">Standard Library Reference</h1>\n'
    )
    f.write(
        '<div class="api-page-description">'
        'Codon standard library modules and APIs.'
        '</div>\n'
    )
    f.write('</header>\n\n')

    write_api_search(f)

    # Classes are the user-oriented API view, so show them before modules.
    write_class_catalog(f, [])

    write_child_modules(
        "",
        f,
        [],
    )


# =============================================================================
# Generate module catalog
# =============================================================================

modules_index_path = os.path.join(
    out_path,
    "modules",
    "index.md",
)

os.makedirs(
    os.path.dirname(modules_index_path),
    exist_ok=True,
)

with open(modules_index_path, "w") as f:
    write_modules_index(f)


# =============================================================================
# Generate class catalog and class pages
# =============================================================================

classes_index_path = os.path.join(
    out_path,
    "classes",
    "index.md",
)

os.makedirs(
    os.path.dirname(classes_index_path),
    exist_ok=True,
)

with open(classes_index_path, "w") as f:
    write_classes_index(f)


for class_info in public_classes:
    output_file = output_path_for_class(class_info)

    os.makedirs(
        os.path.dirname(output_file),
        exist_ok=True,
    )

    with open(output_file, "w") as f:
        write_frontmatter(
            class_info.qualified_name,
            f,
        )
        write_class_page_header(
            class_info,
            f,
        )
        write_consolidated_class(
            class_info,
            f,
        )


# Search is generated after canonical class references have been established.
write_search_index()


# =============================================================================
# Generate module pages
# =============================================================================

for page_name in sorted(all_page_names):
    info = actual_modules.get(page_name)
    is_package = page_name in package_names
    output_file = output_path_for_page(page_name)

    os.makedirs(
        os.path.dirname(output_file),
        exist_ok=True,
    )

    with open(output_file, "w") as f:
        write_frontmatter(
            page_name,
            f,
        )

        write_page_header(
            page_name,
            info.source_rel if info else None,
            is_package,
            f,
        )

        render_module_contents(
            info,
            f,
            page_name,
        )

        write_child_modules(
            page_name,
            f,
        )


print(
    f"Generated {len(all_page_names)} module pages and "
    f"{len(public_classes)} consolidated class pages."
)
print("Done!")
