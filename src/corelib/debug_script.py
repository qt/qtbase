# Copyright (C) 2017 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import os
import sys
import imp

from distutils.version import LooseVersion

MODULE_NAME = 'qt'

debug = print if 'QT_LLDB_SUMMARY_PROVIDER_DEBUG' in os.environ \
    else lambda *a, **k: None

def import_bridge(path, debugger, session_dict, reload_module=False):
    if not reload_module and MODULE_NAME in sys.modules:
        del sys.modules[MODULE_NAME]

    if sys.version_info[0] >= 3:
        sys.path.append(os.path.dirname(path))
    debug(f"Loading source of Qt Creator bridge from '{path}'")
    bridge = imp.load_source(MODULE_NAME, path)

    if not hasattr(bridge, '__lldb_init_module'):
        print("Could not find '__lldb_init_module'. Ignoring.")
        return None

    # Make available for the current LLDB session, so that LLDB
    # can find the functions when initializing the module.
    session_dict[MODULE_NAME] = bridge

    # Initialize the module now that it's available globally
    debug(f"Initializing Qt Creator bridge by calling __lldb_init_module(): {bridge}")
    bridge.__lldb_init_module(debugger, session_dict)

    if not debugger.GetCategory('Qt'):
        debug("Could not find Qt debugger category. Qt Creator summary providers not loaded.")
        # Summary provider failed for some reason
        del session_dict[MODULE_NAME]
        return None

    debug("Bridge loaded successfully")
    return bridge

def __lldb_init_module(debugger, session_dict):
    qtc_env_vars = ['QTC_DEBUGGER_PROCESS', 'QT_CREATOR_LLDB_PROCESS']
    if any(v in os.environ for v in qtc_env_vars) and \
        not 'QT_FORCE_LOAD_LLDB_SUMMARY_PROVIDER' in os.environ:
        debug("Qt Creator lldb bridge not loaded because we're already in a debugging session.")
        return

    # Check if the module has already been imported globally. This ensures
    # that the Qt Creator application search is only performed once per
    # LLDB process invocation, while still reloading for each session.
    if MODULE_NAME in sys.modules:
        module = sys.modules[MODULE_NAME]
        debug(f"Module '{module.__file__}' already imported. Reloading for this session.")
        # Reload module for this sessions
        bridge = import_bridge(module.__file__, debugger, session_dict,
            reload_module = True)
        if bridge:
            debug("Qt summary providers successfully reloaded.")
            return
        else:
            print("Bridge reload failed. Trying to find other Qt Creator bridges.")

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
        debug(f"Loading Qt summary providers from Creator Qt {version} in '{path}'")
        bridge_path = '{}/Contents/Resources/debugger/lldbbridge.py'.format(path)
        bridge = import_bridge(bridge_path, debugger, session_dict)
        if bridge:
            debug(f"Qt summary providers successfully loaded.")
            return

    if 'QT_LLDB_SUMMARY_PROVIDER_DEBUG' not in os.environ:
        print("Could not find any valid Qt Creator bridges with summary providers. "
              "Launch lldb or Qt Creator with the QT_LLDB_SUMMARY_PROVIDER_DEBUG environment "
              "variable to debug.")
