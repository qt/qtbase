#############################################################################
##
## Copyright (C) 2017 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the QtCore module of the Qt Toolkit.
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

import os
import sys
import imp

from distutils.version import LooseVersion

MODULE_NAME = 'qt'

def import_bridge(path, debugger, session_dict, reload_module = False):
    if not reload_module and MODULE_NAME in sys.modules:
        del sys.modules[MODULE_NAME]

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

def report_success(bridge):
    print "Using Qt summary providers from Creator %s in '%s'" \
        % (bridge.CREATOR_VERSION, bridge.CREATOR_PATH)

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
            report_success(bridge)
            return

    versions = {}
    for install in os.popen(
        'mdfind kMDItemCFBundleIdentifier=org.qt-project.qtcreator'
            '| while read p;'
                'do echo $p=$(mdls "$p" -name kMDItemVersion -raw);'
            'done'):
        install = install.strip()
        (p, v) = install.split('=')
        versions[v] = p

    for version in sorted(versions, key=LooseVersion, reverse=True):
        path = versions[version]

        bridge_path = path + '/Contents/Resources/debugger/lldbbridge.py'
        bridge = import_bridge(bridge_path, debugger, session_dict)
        if bridge:
            bridge.CREATOR_VERSION = version
            bridge.CREATOR_PATH = path
            report_success(bridge)
            return

    print "Could not find Qt Creator installation, no Qt summary providers installed"
