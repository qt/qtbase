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

def unicode2hex(s):
    lst = []
    for x in s:
        v = ord(x)
        if v > 0xFFFF:
            # make a surrogate pair
            # copied from qchar.h
            high = (v >> 10) + 0xd7c0
            low = (v % 0x400 + 0xdc00)
            lst.append(hex(high))
            lst.append(hex(low))
        else:
            lst.append(hex(v))
    return lst

def wrap_list(lst):
    def split(lst, size):
        while lst:
            head, lst = lst[:size], lst[size:]
            yield head
    return ",\n".join(", ".join(x) for x in split(lst, 20))


@contextmanager
def AtomicRenameTemporaryFile(originalLocation: Path, *, prefix: str, dir: Path):
    """Context manager for safe file update via a temporary file.

    Accepts path to the file to be updated. Yields a temporary file to the user
    code, open for writing.

    On success closes the temporary file and moves its content to the original
    location. On error, removes temporary file, without disturbing the original.
    """
    tempFile = NamedTemporaryFile('w', prefix=prefix, dir=dir, delete=False)
    try:
        yield tempFile
        tempFile.close()
        # Move the modified file to the original location
        Path(tempFile.name).rename(originalLocation)
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
            self.reader = resources.enter_context(open(self.path))

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
