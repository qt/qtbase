#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
"""
qt-modified-testfunctions.py

Reports C++ test functions added/modified by commits on top of HEAD.

Commit selection can be specified
  --range BASE..HEAD   analyze commits in range
  --last N             analyze last N commits on HEAD

"""

import argparse
import os
import re
import subprocess
import sys
from typing import Dict, Iterator, List, Tuple, Optional, Set
from pathlib import Path

# -------------------------
# Helpers
# -------------------------

def run(cmd: List[str], check: bool = False) -> subprocess.CompletedProcess:
    """Run a command and return the CompletedProcess (stdout, stderr as text)."""
    proc = subprocess.run(cmd, None, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          universal_newlines=True)
    if check and proc.returncode != 0:
        raise RuntimeError(f"Command failed: {' '.join(cmd)}\nSTDOUT:\n{proc.stdout}\nSTDERR:\n{proc.stderr}")
    return proc


def is_test_file(path: str) -> bool:
    path_obj = Path(path)
    return path_obj.name.startswith('tst_') and path_obj.suffix == '.cpp'

# Get list of relevant commits
def get_commits(commit_range: Optional[str], last: Optional[int]) -> List[str]:
    assert not (commit_range and last), "range and last are mutually exclusive (enforced by argparse)"

    if commit_range:
        proc = run(["git", "rev-list", commit_range], check=True)
    elif last is not None:
        proc = run(["git", "rev-list", "--max-count", str(last), "HEAD"], check=True)
    else:
        return []

    commits = [ln.strip() for ln in proc.stdout.splitlines() if ln.strip()]
    return commits

# Parse the diff of a commit
PLUS_RANGE_RE = re.compile(r"\+([0-9]+)(?:,([0-9]+))?")


def extract_new_line_ranges_from_diff(patch_text: str) -> List[Tuple[int, int]]:
    """Return [(start, len)] for +c,d in hunk headers."""
    ranges: List[Tuple[int, int]] = []
    for line in patch_text.splitlines():
        if not line.startswith("@@"):
            continue
        m = PLUS_RANGE_RE.search(line)
        if m:
            start = int(m.group(1))
            length = int(m.group(2)) if m.group(2) else 1
            ranges.append((start, length))
    return ranges


# --- C++ source parsing ----------------------------------------------------
# A changed line is attributed to the function whose brace-matched span contains it, parsed on
# comment/literal-stripped text. This is robust to lambdas, multi-line signatures, nested helper
# classes, preprocessor branches and large bodies.

# Out-of-line member definition `Class::method(...)` (optionally nested or a
# ~destructor) with trailing qualifiers; NS_RE marks the enclosing namespaces.
SIG_RE = re.compile(
    r"([A-Za-z_]\w*(?:::~?[A-Za-z_]\w*)+)\s*\([^;{}]*\)\s*"
    r"(?:const|noexcept|override|final|&|&&|\s)*$"
)
NS_RE = re.compile(r"(^|[^\w])namespace\b")
_HEX = "0123456789abcdefABCDEF"


def strip_comments_and_literals(text: str) -> str:
    """Blank //, /* */ comments and string/char/raw-string *contents* with spaces.

    Keeps newlines and (crucially) column positions, so braces or '::' inside them are never parsed
    as code. The output has the same length as the input.
    """
    out: List[str] = []
    i, n, state = 0, len(text), "code"
    while i < n:
        c = text[i]
        nxt = text[i + 1] if i + 1 < n else ""
        if state == "code":
            if c == "R" and nxt == '"':  # raw string R"delim(...)delim"
                j = i + 2
                while j < n and text[j] != "(":
                    j += 1
                close = ")" + text[i + 2:j] + '"'
                end = text.find(close, j)
                end = n if end == -1 else end
                out.append('R"' + " " * (j - i - 1))  # R", the delimiter and '('
                out.append(re.sub(r"[^\n]", " ", text[j + 1:end + len(close)]))
                i = end + len(close)
            elif c == "/" and nxt == "/":
                state = "line"
                out.append("  ")
                i += 2
            elif c == "/" and nxt == "*":
                state = "block"
                out.append("  ")
                i += 2
            elif c == '"':
                state = "str"
                out.append('"')
                i += 1
            elif c == "'":
                prev = text[i - 1] if i else ""
                if prev and nxt and prev in _HEX and nxt in _HEX:
                    out.append("'")  # C++14 digit separator (0x1f'a2), not a literal
                    i += 1
                else:
                    state = "char"
                    out.append("'")
                    i += 1
            else:
                out.append(c)
                i += 1
        elif state == "line":
            out.append("\n" if c == "\n" else " ")
            if c == "\n":
                state = "code"
            i += 1
        elif state == "block":
            if c == "*" and nxt == "/":
                state = "code"
                out.append("  ")
                i += 2
            else:
                out.append("\n" if c == "\n" else " ")
                i += 1
        else:  # "str" or "char"
            quote = '"' if state == "str" else "'"
            if c == "\\":
                out.append("  ")  # escape: blank both chars
                i += 2
            elif c == quote:
                state = "code"
                out.append(quote)
                i += 1
            else:
                out.append("\n" if c == "\n" else " ")
                i += 1
    return "".join(out)


def find_defs(text: str) -> List[Tuple[str, int, int]]:
    """Return [(qualified_name, start_line, end_line)] for each out-of-line member function.

    Definitions are recognised only at namespace/global scope, so a
    nested helper class's methods and lambdas stay part of their enclosing
    function. A '}' in column 0 closes a top-level construct and resynchronises
    the scope stack, which keeps parsing correct across preprocessor branches
    whose braces don't balance in the raw (both-branches-present) text.
    """
    s = strip_comments_and_literals(text)
    defs: List[Tuple[str, int, int]] = []
    pending: List[Tuple[str, Optional[str], int]] = []  # (kind, name, start_line)
    line, buf, buf_start, i, n = 1, "", 1, 0, len(s)
    while i < n:
        c = s[i]
        if c == "\n":
            line += 1
            buf += " "
            i += 1
        elif c == "#" and not buf.strip():
            # skip a preprocessor directive (and its \ line-continuations) so its
            # tokens / unbalanced braces don't leak into the next signature
            while i < n and s[i] != "\n":
                i += 1
            while i < n and s[i] == "\n" and i and s[i - 1] == "\\":
                line += 1
                i += 1
                while i < n and s[i] != "\n":
                    i += 1
        elif c == "{":
            seg = buf.strip()
            kind, name = "other", None
            if NS_RE.search(seg):
                kind = "ns"
            elif all(k == "ns" for k, _, _ in pending):
                m = SIG_RE.search(seg)
                if m:
                    kind, name = "func", m.group(1)
            pending.append((kind, name, buf_start))
            buf = ""
            i += 1
        elif c == "}":
            # In column 0 (no indent on outer braces) pop back to namespace scope, recording any
            # function(s) closed; otherwise pop one scope.
            top_level = (i == 0 or s[i - 1] == "\n")
            while pending and (top_level and pending[-1][0] != "ns"):
                kind, name, sl = pending.pop()
                if kind == "func" and name:
                    defs.append((name, sl, line))
            if not top_level and pending:
                kind, name, sl = pending.pop()
                if kind == "func" and name:
                    defs.append((name, sl, line))
            buf = ""
            i += 1
        elif c == ";":
            buf = ""
            i += 1
        else:
            if not buf.strip() and not c.isspace():
                buf_start = line
            buf += c
            i += 1
    return defs


def _range_hits_span(start: int, length: int, def_start: int, def_end: int) -> bool:
    """Whether a changed hunk touches the function spanning [def_start, def_end]."""
    if length == 0:
        # pure deletion: removed text sat between new lines start and start+1, so
        # it is interior only when start < def_end (a gap at the '}' is the line
        # after the function, e.g. a deleted neighbour)
        return def_start <= start < def_end
    # inclusive end: a tail modification anchored by git -U0 at the '}' must count
    return start <= def_end and start + length - 1 >= def_start


def functions_for_commit_file(commit: str, path: str, old_path: Optional[str] = None) -> List[str]:
    """Qualified names of functions whose definition text changed in *commit*.

    Every changed line range is attributed to the function whose [start, end] span it touches;
    a candidate is reported only when its definition text really differs from the parent commit,
    which drops unchanged neighbours that a nearby insertion or a file rename/move would otherwise
    flag.
    """
    paths = [old_path, path] if old_path else [path]
    patch = run(["git", "diff", "-U0", "-p", "-M", f"{commit}^!", "--", *paths]).stdout
    ranges = extract_new_line_ranges_from_diff(patch)
    if not ranges:
        return []

    new = run(["git", "show", f"{commit}:{path}"])
    if new.returncode != 0 or not new.stdout:
        return []
    new_lines = new.stdout.splitlines()

    # candidate -> its definition text in the new file
    touched: Dict[str, str] = {}
    for name, s_ln, e_ln in find_defs(new.stdout):
        if "::" in name and any(_range_hits_span(rs, rl, s_ln, e_ln) for rs, rl in ranges):
            touched[name] = "\n".join(new_lines[s_ln - 1:e_ln])
    if not touched:
        return []

    old = run(["git", "show", f"{commit}^:{old_path or path}"])
    old_bodies: Dict[str, str] = {}
    if old.returncode == 0 and old.stdout:
        old_lines = old.stdout.splitlines()
        old_bodies = {name: "\n".join(old_lines[s - 1:e]) for name, s, e in find_defs(old.stdout)}

    return sorted(name for name, text in touched.items() if old_bodies.get(name) != text)


def list_modified_files(commit: str) -> Iterator[Tuple[str, str, Optional[str]]]:
    """Return list of (path, status, old_path) for modified/added/renamed files.
    Status is one of A, M, R*. old_path is set only for renames (R*).
    """
    proc = run(["git", "diff-tree", "--no-commit-id", "--name-status", "-r", "-M", commit])
    for ln in proc.stdout.splitlines():
        # Format: STATUS \t path [\t newpath]
        parts = ln.split("\t")
        if not parts:
            continue
        status = parts[0]
        if status in ("A", "M") and len(parts) >= 2:
            yield (parts[1], status, None)
        elif status.startswith("R") and len(parts) >= 3:
            # rename: new path + old path (old path enables rename detection)
            yield (parts[2], status, parts[1])
        # Other statuses ignored

# -------------------------
# Main Interface
# -------------------------

def main(argv: Optional[List[str]] = None) -> int:
    default_branch = os.getenv("TESTED_MODULE_BRANCH_COIN", default="origin/dev")
    default_range = default_branch + "..HEAD"

    parser = argparse.ArgumentParser(
                description="Report C++ functions modified/added by commits within a given range."
             )
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--range", dest="commit_range", help="Commit range (e.g., BASE..HEAD)",
                       default=None)
    group.add_argument("--last", dest="last", type=int, help="Analyze last N commits on HEAD",
                       default=None)
    parser.add_argument("--output", dest="output", type=Path,
                        help="Write to file instead of stdout", default=None)
    args = parser.parse_args(argv)

    # Apply the default range only when the user gave neither option.
    # --range must NOT carry a non-None default, or it would always be truthy
    # and make --last unusable (get_commits would think both were set).
    if args.commit_range is None and args.last is None:
        args.commit_range = default_range

    print("Modified test function parser started with arguments:")
    if args.commit_range:
        print(f"    --range {args.commit_range}")
    if args.last:
        print(f"    --last {args.last}")
    if args.output:
        print(f"    --output {args.output}")

    try:
        commits = get_commits(args.commit_range, args.last)
    except Exception as e:
        print("Error selecting commits:", e, file=sys.stderr)
        return 2

    if not commits:
        print("No commits found to analyze.", file=sys.stderr)
        return 0

    functions: Set[str] = set()
    for commit in commits:
        for path, _, old_path in list_modified_files(commit):
            if not is_test_file(path):
                continue
            # If this is run in a non .git directory, git returns an error
            # => ignore this, don't crash
            try:
                funcs = functions_for_commit_file(commit, path, old_path)
            except Exception:
                print("This must be run from the top-level directory of a qt module.")
                return 1
            functions.update(funcs)

    if functions:
        filename: Optional[Path] = args.output
        # COIN_CONFIGURE_ARGS is always set in CI
        if filename is None and os.getenv("COIN_CONFIGURE_ARGS"):
            filename = Path(__file__).resolve().parent / "qt-modified-testfunctions.txt"
        if filename is not None:
            with filename.open("w", encoding="utf-8") as out:
                print(f"Writing output to {filename}")
                for fn in sorted(functions):
                    print(fn, file=out)
        else:
            for fn in sorted(functions):
                print(fn, file=sys.stdout)

    size = len(functions)
    print(f"Found {size} modified test functions.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
