
#!/usr/bin/env python3
# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import sys
import struct


class WasmBinary:
    """For reference of binary format see Emscripten source code, especially library_dylink.js."""

    def __init__(self, filepath):
        self._offset = 0
        self._end = 0
        self._dependencies = []
        with open(filepath, 'rb') as file:
            self._binary = file.read()
            self._check_preamble()
            self._parse_subsections()

    def get_dependencies(self):
        return self._dependencies

    def _get_leb(self):
        ret = 0
        mul = 1
        while True:
            byte = self._binary[self._offset]
            self._offset += 1
            ret += (byte & 0x7f) * mul
            mul *= 0x80
            if not (byte & 0x80):
                break
        return ret

    def _get_string(self):
        length = self._get_leb()
        self._offset += length
        return self._binary[self._offset - length:self._offset].decode('utf-8')

    def _check_preamble(self):
        preamble = memoryview(self._binary)[:24]
        int32View = struct.unpack('<6I', preamble)
        assert int32View[0] == 0x6d736100, "magic number not found"
        assert self._binary[8] == 0, "dynlink section needs to be first"
        self._offset = 9
        section_size = self._get_leb()
        self._end = self._offset + section_size
        name = self._get_string()
        assert name == "dylink.0", "section dylink.0 not found"

    def _parse_subsections(self):
        WASM_DYLINK_NEEDED = 0x2

        while self._offset < self._end:
            subsection_type = self._binary[self._offset]
            self._offset += 1
            subsection_size = self._get_leb()

            if subsection_type == WASM_DYLINK_NEEDED:
                needed_dynlibs_count = self._get_leb()
                for _ in range(needed_dynlibs_count):
                    self._dependencies.append(self._get_string())
            else:
                self._offset += subsection_size  # we don't care about other sections for now


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python wasm_binary_tools.py <shared_object>")
        sys.exit(1)

    file_path = sys.argv[1]
    binary = WasmBinary(file_path)
    dependencies = binary.get_dependencies()
    for d in dependencies:
        print(d)
