# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## OhosAppKit Module:
#####################################################################

qt_internal_add_module(OhosAppKit
    EXCEPTIONS
    SOURCES
        qohosabilitycontext.cpp qohosabilitycontext_p.h
        qohosappbundleinfo.cpp qohosappbundleinfo_p.h
        qohosappcontext.cpp qohosappcontext_p.h
        qohosbundlemanager_p.h
        qohosenums_p.h
        qohosfileshare.cpp qohosfileshare_p.h
        qohosfileutils.cpp qohosfileutils_p.h
        qohosjsenv_p.h
        qohosoperationstatus.cpp qohosoperationstatus_p.h
        qohospasteboard.cpp qohospasteboard_p.h
        qohossharekit.cpp qohossharekit_p.h
        qohossharekitbackend.cpp qohossharekitbackend_p.h
        qohosstartoptions.cpp qohosstartoptions_p.h
        qohoswant.cpp qohoswant_p.h
        qohoswantinfo.cpp qohoswantinfo_p.h
        qohoswantutils.cpp qohoswantutils_p.h
        qohoswindowutils.cpp qohoswindowutils_p.h
        qtohosappkitglobal_p.h
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
