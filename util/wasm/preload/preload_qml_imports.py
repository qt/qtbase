#!/usr/bin/env python3
# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import os
import sys
import subprocess
import json
import re

# Paths to shared libraries and qml imports on the Qt installation on the web server.
# "$QTDIR" is replaced by qtloader.js at load time (defaults to "qt"), and makes
# possible to relocate the application build relative to the Qt build on the web server.
qt_lib_path = "$QTDIR/lib"
qt_qml_path = "$QTDIR/qml"

# Path to QML imports on the in-memory file system provided by Emscripten. This script emits
# preload commands which copies QML imports to this directory. In addition, preload_qt_plugins.py
# creates (and preloads) a qt.conf file which makes Qt load QML plugins from this location.
qt_deploy_qml_path = "/qt/qml"


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


def preload_file(source, destination):
    preload_files.append({"source": source, "destination": destination})


def find_dependencies(filepath):
    # Very basic dependency finder which scans for ".so" strings in the file
    try:
        with open(filepath, "rb") as file:
            content = file.read()
            return [
                m.group(0).decode("utf-8")
                for m in re.finditer(rb"[\w\-.]+\.so", content)
            ]
    except IOError as e:
        eprint(f"Error: {e}")
        return []


def extract_preload_files_from_imports(imports):
    libraries = []
    files = []
    for qml_import in imports:
        try:
            relative_path = qml_import["relativePath"]
            plugin = qml_import["plugin"]

            # plugin .so
            so_plugin_source_path = os.path.join(
                qt_qml_path, relative_path, "lib" + plugin + ".so"
            )
            so_plugin_destination_path = os.path.join(
                qt_deploy_qml_path, relative_path, "lib" + plugin + ".so"
            )

            preload_file(so_plugin_source_path, so_plugin_destination_path)
            so_plugin_qt_install_path = os.path.join(
                qt_wasm_path, "qml", relative_path, "lib" + plugin + ".so"
            )
            deps = find_dependencies(so_plugin_qt_install_path)
            libraries.extend(deps)

            # qmldir file
            qmldir_source_path = os.path.join(qt_qml_path, relative_path, "qmldir")
            qmldir_destination_path = os.path.join(
                qt_deploy_qml_path, relative_path, "qmldir"
            )
            preload_file(qmldir_source_path, qmldir_destination_path)
        except Exception as e:
            eprint(e)
            continue
    return files, libraries


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python make_qt_symlinks.py <qt-host-path> <qt-wasm-path>")
        sys.exit(1)

    qt_host_path = sys.argv[1]
    qt_wasm_path = sys.argv[2]

    qml_import_path = os.path.join(qt_wasm_path, "qml")
    qmlimportsscanner_path = os.path.join(qt_host_path, "libexec/qmlimportscanner")

    eprint("runing qmlimportsscanner")
    result = subprocess.run(
        [qmlimportsscanner_path, "-rootPath", ".", "-importPath", qml_import_path],
        stdout=subprocess.PIPE,
    )
    imports = json.loads(result.stdout)

    preload_files = []
    libraries = []
    files, libraries = extract_preload_files_from_imports(imports)

    # Deploy plugin dependencies, that is, shared libraries used by the plugins.
    # Skip some of the obvious libraries which will be
    skip_libraries = [
        "libQt6Core.so",
        "libQt6Gui.so",
        "libQt6Quick.so",
        "libQt6Qml.so" "libQt6Network.so",
        "libQt6OpenGL.so",
    ]

    libraries = set(libraries) - set(skip_libraries)
    for library in libraries:
        source = os.path.join(qt_lib_path, library)
        # Emscripten looks for shared libraries on "/", shared libraries
        # most be deployed there instead of at /qt/lib
        destination = os.path.join("/", library)
        preload_file(source, destination)

    print(json.dumps(preload_files, indent=2))
