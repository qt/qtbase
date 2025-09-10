#!/usr/bin/env python3
# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import os
import sys
import json

# Path to plugins on the Qt installation on the web server. "$QTPATH" is replaced by qtloader.js
# at load time (defaults to "qt"), which makes it possible to relocate the application build relative
# to the Qt build on the web server.
qt_plugins_path = "$QTDIR/plugins"

# Path to plugins on the in-memory file system provided by Emscripten. This script emits
# preload commands which copies plugins to this directory.
qt_deploy_plugins_path = "/qt/plugins"


def find_so_files(directory):
    so_files = []
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith(".so"):
                relative_path = os.path.relpath(os.path.join(root, file), directory)
                so_files.append(relative_path)
    return so_files


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python make_qt_symlinks.py <qt-wasm-path>")
        sys.exit(1)

    qt_wasm_path = sys.argv[1]

    # preload all plugins
    plugins = find_so_files(os.path.join(qt_wasm_path, "plugins"))
    preload = [
        {
            "source": os.path.join(qt_plugins_path, plugin),
            "destination": os.path.join(qt_deploy_plugins_path, plugin),
        }
        for plugin in plugins
    ]

    # Create and preload qt.conf which will tell Qt to look for plugins
    # and QML imports in /qt/plugins and /qt/qml. The qt.conf file is
    # written to the current directory.
    qtconf = "[Paths]\nPrefix = /qt\n"
    with open("qt.conf", "w") as f:
        f.write(qtconf)
    preload.append({"source": "qt.conf", "destination": "/qt.conf"})

    print(json.dumps(preload, indent=2))
