# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tracegen Tool:
#####################################################################

qt_get_tool_target_name(target_name tracegen)
qt_internal_add_tool(${target_name}
    CORE_LIBRARY Bootstrap
    INSTALL_DIR "${INSTALL_LIBEXECDIR}"
    TOOLS_TARGET Core
    SOURCES
        etw.cpp etw.h
        helpers.cpp helpers.h
        ctf.cpp ctf.h
        lttng.cpp lttng.h
        panic.cpp panic.h
        provider.cpp provider.h
        qtheaders.cpp qtheaders.h
        tracegen.cpp
)
qt_internal_return_unless_building_tools()

set_target_properties(${target_name} PROPERTIES
    UNITY_BUILD OFF
)
