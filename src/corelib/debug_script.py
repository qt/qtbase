# Copyright (C) 2017 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import os
import sys
import imp

from distutils.version import LooseVersion

MODULE_NAME = 'qt'

def import_bridge(path, debugger, session_dict, reload_module=False):
    if not reload_module and MODULE_NAME in sys.modules:
        del sys.modules[MODULE_NAME]

    if sys.version_info[0] >= 3:
        sys.path.append(os.path.dirname(path))
    bridge = imp.load_source(MODULE_NAME, path)

    if not hasattr(bridge, '__lldb_init_module'):
        return None

    # Make available for the current LLDB session, so that LLDB
    # can find the functions when initializing the module.
    session_dict[MODULE_NAME] = bridge

    # Initialize the module now that it's available globally
    bridge.__lldb_init_module(debugger, session_dict)

    if not debugger.GetCategory('Qt'):
        # Summary provider failed for some reason
        del session_dict[MODULE_NAME]
        return None

    return bridge

def __lldb_init_module(debugger, session_dict):
    # Check if the module has already been imported globally. This ensures
    # that the Qt Creator application search is only performed once per
    # LLDB process invocation, while still reloading for each session.
    if MODULE_NAME in sys.modules:
        module = sys.modules[MODULE_NAME]
        # Reload module for this sessions
        bridge = import_bridge(module.__file__, debugger, session_dict,
            reload_module = True)
        if bridge:
            return

    versions = {}
    for path in os.popen('mdfind kMDItemCFBundleIdentifier=org.qt-project.qtcreator'):
        path = path.strip()
        file = open(os.path.join(path, 'Contents', 'Info.plist'), "rb")

        import plistlib
        plist = plistlib.load(file)

        version = None
        for key in ["CFBundleVersion", "CFBundleShortVersionString"]:
            if key in plist:
                version = plist[key]
                break

        if not version:
            print(f"Could not resolve version for '{path}'. Ignoring.")
            continue

        versions[version] = path

    if not len(versions):
        print("Could not find Qt Creator installation. No Qt summary providers installed.")
        return

    for version in sorted(versions, key=LooseVersion, reverse=True):
        path = versions[version]
        print(f"Loading Qt summary providers from Creator {version} in '{path}'")
        bridge_path = '{}/Contents/Resources/debugger/lldbbridge.py'.format(path)
        bridge = import_bridge(bridge_path, debugger, session_dict)
        if bridge:
            return
