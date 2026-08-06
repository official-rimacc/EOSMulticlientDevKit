#!/usr/bin/env python3
"""Fail if two .cpp files in the same module define the same file-scope symbol.

Why this exists
---------------
UnrealBuildTool concatenates several .cpp files into one translation unit (a
"unity" blob). File-scope statics, and members of anonymous namespaces, from the
constituent files then share a single scope and collide:

    error C2374: 'X': redefinition; multiple initialization
    error C2086: 'const TCHAR *X': redefinition
    error C2084: function 'bool f(const FString &)' already has a body

Anonymous namespaces do not help. Two anonymous namespaces in one translation
unit are the same namespace.

UBT's adaptive unity excludes recently-modified files from unity blobs, so any
working copy where the offending files have been touched compiles fine. Only an
untouched tree with no Intermediate/ bundles them, which is precisely the fresh
clone case. That is why this shipped broken in 1.0.0.

This check needs no engine and no compiler, so it runs on a stock CI runner.

Usage: check_unity_symbol_collisions.py [source-root ...]
"""

import re
import sys
from collections import defaultdict
from pathlib import Path

# A definition of a named entity: optional storage/qualifier words, a type, then
# the name, then '=' (variable), ';' (variable) or '(' (function).
DEFINITION = re.compile(
    r"^(?P<prefix>(?:(?:static|const|constexpr|inline|extern|thread_local|mutable)\s+)*)"
    r"(?P<type>[A-Za-z_][A-Za-z0-9_:<>,\s]*[\s\*&]+)"
    r"(?P<name>[A-Za-z_]\w*)\s*"
    r"(?P<tail>[=;(])"
)

# struct/class/union/enum definitions (not forward declarations).
TYPE_DEFINITION = re.compile(
    r"^(?:struct|class|union|enum(?:\s+class)?)\s+(?:\w+_API\s+)?(?P<name>[A-Za-z_]\w*)"
    r"\s*(?:final\s*)?(?:$|[{:])"
)

NAMESPACE_OPEN = re.compile(r"^namespace(?:\s+(?P<name>[A-Za-z_][\w:]*))?\s*(?P<brace>\{)?\s*$")


def strip_comments_and_strings(text):
    """Blank out comments and literal contents, preserving line numbering."""
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        nxt = text[i + 1] if i + 1 < n else ""
        if c == "/" and nxt == "/":
            while i < n and text[i] != "\n":
                i += 1
        elif c == "/" and nxt == "*":
            i += 2
            while i < n and not (text[i] == "*" and i + 1 < n and text[i + 1] == "/"):
                if text[i] == "\n":
                    out.append("\n")
                i += 1
            i += 2
        elif c in "\"'":
            quote = c
            out.append(quote * 2)
            i += 1
            while i < n:
                if text[i] == "\\":
                    i += 2
                    continue
                if text[i] == quote:
                    i += 1
                    break
                if text[i] == "\n":
                    break
                i += 1
        else:
            out.append(c)
            i += 1
    return "".join(out)


def scan(path):
    """Yield (key, line_number) for every symbol this .cpp defines at file scope
    or directly inside a file-scope namespace."""
    text = strip_comments_and_strings(path.read_text(encoding="utf-8", errors="replace"))

    # Each open brace pushes an entry. ('ns', name) for a namespace body,
    # ('other', None) for anything else (function body, struct body, ...).
    stack = []
    pending_ns = None  # namespace name awaiting its '{', '' when anonymous

    for lineno, raw in enumerate(text.split("\n"), 1):
        line = raw.strip()

        if not line or line.startswith("#"):
            _, stack, pending_ns = consume_braces(line, stack, pending_ns)
            continue

        ns_open = NAMESPACE_OPEN.match(line)
        if ns_open:
            pending_ns = ns_open.group("name") or ""
            if ns_open.group("brace"):
                stack.append(("ns", pending_ns))
                pending_ns = None
            continue

        # Only file scope, or one level into a file-scope namespace, can collide.
        scope = None
        if not stack:
            scope = ""
        elif len(stack) == 1 and stack[0][0] == "ns":
            scope = stack[0][1] + "::"

        if scope is not None:
            name = extract_name(line)
            if name:
                yield scope + name, lineno

        _, stack, pending_ns = consume_braces(line, stack, pending_ns)

    return


def extract_name(line):
    type_def = TYPE_DEFINITION.match(line)
    if type_def and not line.rstrip().endswith(";"):
        return type_def.group("name")

    match = DEFINITION.match(line)
    if not match:
        return None

    # 'void SEOSSetupTab::Construct(...)' defines a member, not a file-scope
    # symbol. The qualifier lands at the end of the type group.
    if match.group("type").rstrip().endswith("::"):
        return None

    # 'return Foo(...)' and friends are statements, not definitions. Anything at
    # the scopes we inspect that starts with a keyword like this is not a
    # definition we care about.
    first = line.split(None, 1)[0]
    if first in {"return", "if", "for", "while", "switch", "else", "using", "typedef", "friend"}:
        return None

    return match.group("name")


def consume_braces(line, stack, pending_ns):
    for ch in line:
        if ch == "{":
            if pending_ns is not None:
                stack.append(("ns", pending_ns))
                pending_ns = None
            else:
                stack.append(("other", None))
        elif ch == "}":
            if stack:
                stack.pop()
    return line, stack, pending_ns


def main(argv):
    roots = [Path(a) for a in argv[1:]] or [Path("Source")]

    modules = []
    for root in roots:
        if not root.exists():
            print(f"error: no such path: {root}", file=sys.stderr)
            return 2
        modules.extend(sorted({p.parent for p in root.rglob("*.Build.cs")}))

    if not modules:
        print("error: found no *.Build.cs, so nothing to check", file=sys.stderr)
        return 2

    failures = 0
    for module in modules:
        sources = sorted(module.rglob("*.cpp"))
        seen = defaultdict(list)
        for src in sources:
            for key, lineno in scan(src):
                seen[key].append((src, lineno))

        collisions = {k: v for k, v in seen.items() if len({p for p, _ in v}) > 1}

        print(f"{module.name}: {len(sources)} .cpp file(s), {len(seen)} file-scope symbol(s)")
        if not collisions:
            continue

        failures += len(collisions)
        for key, sites in sorted(collisions.items()):
            print(f"\n  DUPLICATE: {key}")
            for path, lineno in sites:
                print(f"    {path.as_posix()}:{lineno}")

    if failures:
        print(
            f"\nFAILED: {failures} symbol(s) defined in more than one .cpp of the same module.\n"
            "These collide once UnrealBuildTool merges the files into one unity blob, which is\n"
            "what a fresh clone with no Intermediate/ does. Move each shared symbol into one\n"
            "private header as 'inline' (functions) or 'inline constexpr' (constants) and delete\n"
            "the duplicates. Do not paper over it with bUseUnity = false.",
            file=sys.stderr,
        )
        return 1

    print("\nOK: no file-scope symbol is defined in more than one .cpp of a module.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
