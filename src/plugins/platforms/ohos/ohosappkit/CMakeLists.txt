# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## OhosAppKit Module:
#####################################################################

qt_internal_add_module(OhosAppKit
    EXCEPTIONS
    SOURCES
        qohosenums_p.h
        qohosjsenv_p.h
        qohosqpafunctionspart1.cpp qohosqpafunctionspart1_p.h
        qohossharekitbackend.cpp qohossharekitbackend_p.h
    DEFINES
        NODE_ADDON_API_ENABLE_TYPE_CHECK_ON_AS
        NAPI_CPP_EXCEPTIONS
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        NodeAddonApi::NodeAddonApi
        ace_napi.z
        hilog_ndk.z
        ohfileshare
        ohfileuri
        udmf
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Widgets
)
