# Copyright (C) 2020 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
"""Utilities shared among the CLDR extraction tools.

Functions:
  unicode2hex() -- converts unicode text to UCS-2 in hex form.
  wrap_list() -- map list to comma-separated string, 20 entries per line.

Classes:
  Error -- A shared error class.
  Transcriber -- edit a file by writing a temporary file, then renaming.
  SourceFileEditor -- adds standard prelude and tail handling to Transcriber.
"""

from contextlib import ExitStack, contextmanager
from pathlib import Path
from tempfile import NamedTemporaryFile

qtbase_root = Path(__file__).parents[2]
assert qtbase_root.name == 'qtbase'

class Error (Exception):
    def __init__(self, msg, *args):
        super().__init__(msg, *args)
        self.message = msg
    def __str__(self):
        return self.message

def qtVersion(root = qtbase_root, pfx = 'set(QT_REPO_MODULE_VERSION '):
    with open(root.joinpath('.cmake.conf')) as fd:
        for line in fd:
            if line.startswith(pfx):
                tail = line[len(pfx):].strip()
                assert tail, ('No Qt version given', line)
                if tail.startswith('"') or tail.startswith("'"):
                    cut = tail.index(tail[0], 1) # assert: doesn't ValueError
                    assert cut > 5, ('Truncated Qt version', tail)
                    version = tail[1:cut].strip()
                    assert all(x.isdigit() for x in version.split('.')), version
                    return version
                raise Error(f'Missing quotes on Qt version: {tail}')
    raise Error(f'Failed to find {pfx}...) line in {root.joinpath(".cmake.conf")}')
qtVersion = qtVersion()

def unicode2hex(s: str) -> list[str]:
    lst: list[str] = []
    for x in s:
        v: int = ord(x)
        if v > 0xFFFF:
            # make a surrogate pair
            # copied from qchar.h
            high: int = (v >> 10) + 0xd7c0
            low: int = (v % 0x400 + 0xdc00)
            lst.append(hex(high))
            lst.append(hex(low))
        else:
            lst.append(hex(v))
    return lst

def wrap_list(lst, perline=20):
    def split(lst, size):
        while lst:
            head, lst = lst[:size], lst[size:]
            yield head
    return ",\n".join(", ".join(x) for x in split(lst, perline))

def names_clash(cldr: str, enum: str) -> None | str:
    """True if the reader might not recognize cldr as the name of enum

    First argument, cldr, is the name CLDR gives for some language,
    script or territory; second, enum, is the name enumdata.py gives
    for it. If these are enough alike, returns None; otherwise, a
    non-empty string that results from adapting cldr to be more like
    how enumdata.py would express it."""
    if cldr == enum:
        return None

    # Some common substitutions:
    cldr = cldr.replace('&', 'And')
    prefix = { 'St.': 'Saint', 'U.S.': 'United States' }
    for k, v in prefix.items():
        if cldr.startswith(k + ' '):
            cldr = v + cldr[len(k):]

    # Chop out any parenthesised part, e.g. (Burma):
    while '(' in cldr:
        try:
            f, t = cldr.index('('), cldr.rindex(')')
        except ValueError:
            break
        cldr = cldr[:f].rstrip() + ' ' + cldr[t + 1:].lstrip()

    # Various accented letters:
    remap = { 'ã': 'a', 'å': 'a', 'ā': 'a', 'ç': 'c', 'é': 'e', 'í': 'i', 'ô': 'o', 'ü': 'u'}
    skip = '\u02bc' # Punctuation for which .isalpha() is true.
    # Let cldr match (ignoring non-letters and case) any substring as enum:
    if ''.join(enum.lower().split()) in ''.join(
            remap.get(ch, ch) for ch in cldr.lower() if ch.isalpha() and ch not in skip):
        return None
    return cldr


@contextmanager
def AtomicRenameTemporaryFile(originalLocation: Path, *, prefix: str, dir: Path):
    """Context manager for safe file update via a temporary file.

    Accepts path to the file to be updated. Yields a temporary file to the user
    code, open for writing.

    On success closes the temporary file and moves its content to the original
    location. On error, removes temporary file, without disturbing the original.
    """
    tempFile = NamedTemporaryFile('w', prefix=prefix, dir=dir, delete=False, encoding='utf-8')
    try:
        yield tempFile
        tempFile.close()
        # Move the modified file to the original location
        Path(tempFile.name).replace(originalLocation)
    except Exception:
        # delete the temporary file in case of error
        tempFile.close()
        Path(tempFile.name).unlink()
        raise


class Transcriber:
    """Context manager base-class to manage source file rewrites.

    Derived classes need to implement transcribing of the content, with
    whatever modifications they may want.  Members reader and writer
    are exposed; use writer.write() to output to the new file; use
    reader.readline() or iterate reader to read the original.

    This class is intended to be used as context manager only (inside a
    `with` statement).

    Reimplement onEnter() to write any preamble the file may have,
    onExit() to write any tail. The body of the with statement takes
    care of anything in between, using methods provided by derived classes.

    The data is written to a temporary file first. The temporary file data
    is then moved to the original location if there were no errors. Otherwise
    the temporary file is removed and the original is left unchanged.
    """
    def __init__(self, path: Path, temp_dir: Path):
        self.path = path
        self.tempDir = temp_dir

    def onEnter(self) -> None:
        """
        Called before transferring control to user code.

        This function can be overridden in derived classes to perform actions
        before transferring control to the user code.

        The default implementation does nothing.
        """
        pass

    def onExit(self) -> None:
        """
        Called after return from user code.

        This function can be overridden in derived classes to perform actions
        after successful return from user code.

        The default implementation does nothing.
        """
        pass

    def __enter__(self):
        with ExitStack() as resources:
            # Create a temp file to write the new data into
            self.writer = resources.enter_context(
                AtomicRenameTemporaryFile(self.path, prefix=self.path.name, dir=self.tempDir))
            # Open the old file
            self.reader = resources.enter_context(open(self.path, encoding='utf-8'))

            self.onEnter()

            # Prevent resources from being closed on normal return from this
            # method and make them available inside __exit__():
            self.__resources = resources.pop_all()
            return self

    def __exit__(self, exc_type, exc_value, traceback):
        if exc_type is None:
            with self.__resources:
               self.onExit()
        else:
            self.__resources.__exit__(exc_type, exc_value, traceback)

        return False


class SourceFileEditor (Transcriber):
    """Transcriber with transcription of code around a gnerated block.

    We have a common pattern of source files with a generated part
    embedded in a context that's not touched by the regeneration
    scripts. The generated part is, in each case, marked with a common
    pair of start and end markers. We transcribe the old file to a new
    temporary file; on success, we then remove the original and move
    the new version to replace it.

    This class takes care of transcribing the parts before and after
    the generated content; on entering the context, an instance will
    copy the preamble up to the start marker; on exit from the context
    it will skip over the original's generated content and resume
    transcribing with the end marker.

    This class is only intended to be used as a context manager:
    see Transcriber. Derived classes implement suitable methods for use in
    the body of the with statement, using self.writer to rewrite the part
    of the file between the start and end markers.
    """
    GENERATED_BLOCK_START = '// GENERATED PART STARTS HERE'
    GENERATED_BLOCK_END = '// GENERATED PART ENDS HERE'

    def onEnter(self) -> None:
        # Copy over the first non-generated section to the new file
        for line in self.reader:
            self.writer.write(line)
            if line.strip() == self.GENERATED_BLOCK_START:
                break

    def onExit(self) -> None:
        # Skip through the old generated data in the old file
        for line in self.reader:
            if line.strip() == self.GENERATED_BLOCK_END:
                self.writer.write(line)
                break
        # Transcribe the remainder:
        for line in self.reader:
            self.writer.write(line)
