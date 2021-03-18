#############################################################################
##
## Copyright (C) 2020 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the test suite of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:GPL-EXCEPT$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3 as published by the Free Software
## Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################
"""Utilities shared among the CLDR extraction tools.

Functions:
  unicode2hex() -- converts unicode text to UCS-2 in hex form.
  wrap_list() -- map list to comma-separated string, 20 entries per line.

Classes:
  Error -- A shared error class.
  Transcriber -- edit a file by writing a temporary file, then renaming.
  SourceFileEditor -- adds standard prelude and tail handling to Transcriber.
"""

import os
import tempfile

class Error (StandardError):
    __upinit = StandardError.__init__
    def __init__(self, msg, *args):
        self.__upinit(msg, *args)
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

class Transcriber (object):
    """Helper class to facilitate rewriting source files.

    This class takes care of the temporary file manipulation. Derived
    classes need to implement transcribing of the content, with
    whatever modifications they may want.  Members reader and writer
    are exposed; use writer.write() to output to the new file; use
    reader.readline() or iterate reader to read the original.

    Callers should call close() on success or cleanup() on failure (to
    clear away the temporary file).
    """
    def __init__(self, path, temp):
        # Open the old file
        self.reader = open(path)
        # Create a temp file to write the new data into
        temp, tempPath = tempfile.mkstemp(os.path.split(path)[1], dir = temp)
        self.__names = path, tempPath
        self.writer = os.fdopen(temp, "w")

    def close(self):
        self.reader.close()
        self.writer.close()
        self.reader = self.writer = None
        source, temp = self.__names
        os.remove(source)
        os.rename(temp, source)

    def cleanup(self):
        if self.__names:
            self.reader.close()
            self.writer.close()
            # Remove temp-file:
            os.remove(self.__names[1])
            self.__names = ()

class SourceFileEditor (Transcriber):
    """Transcriber with transcription of code around a gnerated block.

    We have a common pattern of source files with a generated part
    embedded in a context that's not touched by the regeneration
    scripts. The generated part is, in each case, marked with a common
    pair of start and end markers. We transcribe the old file to a new
    temporary file; on success, we then remove the original and move
    the new version to replace it.

    This class takes care of transcribing the parts before and after
    the generated content; on creation, an instance will copy the
    preamble up to the start marker; its close() will skip over the
    original's generated content and resume transcribing with the end
    marker. Derived classes need only implement the generation of the
    content in between.

    Callers should call close() on success or cleanup() on failure (to
    clear away the temporary file); see Transcriber.
    """
    __upinit = Transcriber.__init__
    def __init__(self, path, temp):
        """Set up the source file editor.

        Requires two arguments: the path to the source file to be read
        and, on success, replaced with a new version; and the
        directory in which to store the temporary file during the
        rewrite."""
        self.__upinit(path, temp)
        self.__copyPrelude()

    __upclose = Transcriber.close
    def close(self):
        self.__copyTail()
        self.__upclose()

    # Implementation details:
    GENERATED_BLOCK_START = '// GENERATED PART STARTS HERE'
    GENERATED_BLOCK_END = '// GENERATED PART ENDS HERE'

    def __copyPrelude(self):
        # Copy over the first non-generated section to the new file
        for line in self.reader:
            self.writer.write(line)
            if line.strip() == self.GENERATED_BLOCK_START:
                break

    def __copyTail(self):
        # Skip through the old generated data in the old file
        for line in self.reader:
            if line.strip() == self.GENERATED_BLOCK_END:
                self.writer.write(line)
                break
        # Transcribe the remainder:
        for line in self.reader:
            self.writer.write(line)
