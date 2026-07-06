# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## OhosAppKit Module:
#####################################################################

qt_internal_add_module(OhosAppKit
    EXCEPTIONS
    SOURCES
        qohosabilitycontext.cpp qohosabilitycontext.h
        qohosappbundleinfo.cpp qohosappbundleinfo.h qohosappbundleinfo_p.h
        qohosappcontext.cpp qohosappcontext.h
        qohosbundlemanager.h
        qohosenums_p.h
        qohosfileshare.cpp qohosfileshare.h
        qohosjsenv_p.h
        qohosoperationstatus.cpp qohosoperationstatus.h qohosoperationstatus_p.h
        qohospasteboard.cpp qohospasteboard.h
        qohosqpafunctionspart1.cpp qohosqpafunctionspart1_p.h
        qohossharekit.cpp qohossharekit.h qohossharekit_p.h
        qohossharekitbackend.cpp qohossharekitbackend_p.h
        qohosstartoptions.cpp qohosstartoptions.h qohosstartoptions_p.h
        qohosstartrequest.cpp qohosstartrequest.h qohosstartrequest_p.h
        qohosuiabilitycontext.h
        qohoswant.cpp qohoswant.h
        qohoswantutils.cpp qohoswantutils_p.h
        qtohosappkitglobal.h
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
