# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QWindowsDirect2DIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QWindowsDirect2DIntegrationPlugin
    OUTPUT_NAME qdirect2d
    PLUGIN_TYPE platforms
    SOURCES
        ../windows/qtwindowsglobal.h
        ../windows/qwin10helpers.cpp ../windows/qwin10helpers.h
        ../windows/qwindowsapplication.cpp ../windows/qwindowsapplication.h
        ../windows/qwindowscombase.h
        ../windows/qwindowscontext.cpp ../windows/qwindowscontext.h
        ../windows/qwindowscursor.cpp ../windows/qwindowscursor.h
        ../windows/qwindowsdialoghelpers.cpp ../windows/qwindowsdialoghelpers.h
        ../windows/qwindowsdropdataobject.cpp ../windows/qwindowsdropdataobject.h
        ../windows/qwindowsinputcontext.cpp ../windows/qwindowsinputcontext.h
        ../windows/qwindowsintegration.cpp ../windows/qwindowsintegration.h
        ../windows/qwindowsinternalmimedata.cpp ../windows/qwindowsinternalmimedata.h
        ../windows/qwindowskeymapper.cpp ../windows/qwindowskeymapper.h
        ../windows/qwindowsmenu.cpp ../windows/qwindowsmenu.h
        ../windows/qwindowsmimeregistry.cpp ../windows/qwindowsmimeregistry.h
        ../windows/qwindowsmousehandler.cpp ../windows/qwindowsmousehandler.h
        ../windows/qwindowsnativeinterface.cpp ../windows/qwindowsnativeinterface.h
        ../windows/qwindowsole.cpp ../windows/qwindowsole.h
        ../windows/qwindowsopengltester.cpp ../windows/qwindowsopengltester.h
        ../windows/qwindowspointerhandler.cpp ../windows/qwindowspointerhandler.h
        ../windows/qwindowsscreen.cpp ../windows/qwindowsscreen.h
        ../windows/qwindowsservices.cpp ../windows/qwindowsservices.h
        ../windows/qwindowstheme.cpp ../windows/qwindowstheme.h
        ../windows/qwindowsthreadpoolrunner.h
        ../windows/qwindowswindow.cpp ../windows/qwindowswindow.h
        qwindowsdirect2dbackingstore.cpp qwindowsdirect2dbackingstore.h
        qwindowsdirect2dbitmap.cpp qwindowsdirect2dbitmap.h
        qwindowsdirect2dcontext.cpp qwindowsdirect2dcontext.h
        qwindowsdirect2ddevicecontext.cpp qwindowsdirect2ddevicecontext.h
        qwindowsdirect2dhelpers.h
        qwindowsdirect2dintegration.cpp qwindowsdirect2dintegration.h
        qwindowsdirect2dnativeinterface.cpp qwindowsdirect2dnativeinterface.h
        qwindowsdirect2dpaintdevice.cpp qwindowsdirect2dpaintdevice.h
        qwindowsdirect2dpaintengine.cpp qwindowsdirect2dpaintengine.h
        qwindowsdirect2dplatformpixmap.cpp qwindowsdirect2dplatformpixmap.h
        qwindowsdirect2dplatformplugin.cpp
        qwindowsdirect2dwindow.cpp qwindowsdirect2dwindow.h
    NO_UNITY_BUILD_SOURCES
        ../windows/qwindowspointerhandler.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_FOREACH
    INCLUDE_DIRECTORIES
        ../windows
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        advapi32
        d2d1
        d3d11
        dwmapi
        dwrite
        dxgi
        dxguid
        gdi32
        imm32
        ole32
        oleaut32
        setupapi
        shell32
        shlwapi
        user32
        version
        winmm
        winspool
        wtsapi32
        shcore
        comdlg32
        d3d9
        runtimeobject
)

# Resources:
set_source_files_properties("../windows/openglblacklists/default.json"
    PROPERTIES QT_RESOURCE_ALIAS "default.json"
)
set(openglblacklists_resource_files
    "../windows/openglblacklists/default.json"
)

qt_internal_add_resource(QWindowsDirect2DIntegrationPlugin "openglblacklists"
    PREFIX
        "/qt-project.org/windows/openglblacklists"
    FILES
        ${openglblacklists_resource_files}
)

## Scopes:
#####################################################################

qt_internal_extend_target(QWindowsDirect2DIntegrationPlugin CONDITION QT_FEATURE_opengl AND NOT QT_FEATURE_dynamicgl
    LIBRARIES
        opengl32
)

qt_internal_extend_target(QWindowsDirect2DIntegrationPlugin CONDITION MINGW
    LIBRARIES
        uuid
    NO_PCH_SOURCES
        ../windows/qwindowspointerhandler.cpp
)

qt_internal_extend_target(QWindowsDirect2DIntegrationPlugin CONDITION QT_FEATURE_opengl
    SOURCES
        ../windows/qwindowsglcontext.cpp ../windows/qwindowsglcontext.h
        ../windows/qwindowsopenglcontext.h
)

qt_internal_extend_target(QWindowsDirect2DIntegrationPlugin CONDITION QT_FEATURE_systemtrayicon
    SOURCES
        ../windows/qwindowssystemtrayicon.cpp ../windows/qwindowssystemtrayicon.h
)

qt_internal_extend_target(QWindowsDirect2DIntegrationPlugin CONDITION QT_FEATURE_vulkan
    SOURCES
        ../windows/qwindowsvulkaninstance.cpp ../windows/qwindowsvulkaninstance.h
)

qt_internal_extend_target(QWindowsDirect2DIntegrationPlugin CONDITION QT_FEATURE_clipboard
    SOURCES
        ../windows/qwindowsclipboard.cpp ../windows/qwindowsclipboard.h
)

qt_internal_extend_target(QWindowsDirect2DIntegrationPlugin CONDITION QT_FEATURE_clipboard AND QT_FEATURE_draganddrop
    SOURCES
        ../windows/qwindowsdrag.cpp ../windows/qwindowsdrag.h
)

qt_internal_extend_target(QWindowsDirect2DIntegrationPlugin CONDITION QT_FEATURE_tabletevent
    SOURCES
        ../windows/qwindowstabletsupport.cpp ../windows/qwindowstabletsupport.h
    INCLUDE_DIRECTORIES
        ${QtBase_SOURCE_DIR}/src/3rdparty/wintab
)

qt_internal_extend_target(QWindowsDirect2DIntegrationPlugin CONDITION QT_FEATURE_sessionmanager
    SOURCES
        ../windows/qwindowssessionmanager.cpp ../windows/qwindowssessionmanager.h
)

if(QT_FEATURE_imageformat_png)
    # Resources:
    set(cursors_resource_files
        "../windows/images/closedhandcursor_32.png"
        "../windows/images/closedhandcursor_48.png"
        "../windows/images/closedhandcursor_64.png"
        "../windows/images/dragcopycursor_32.png"
        "../windows/images/dragcopycursor_48.png"
        "../windows/images/dragcopycursor_64.png"
        "../windows/images/draglinkcursor_32.png"
        "../windows/images/draglinkcursor_48.png"
        "../windows/images/draglinkcursor_64.png"
        "../windows/images/dragmovecursor_32.png"
        "../windows/images/dragmovecursor_48.png"
        "../windows/images/dragmovecursor_64.png"
        "../windows/images/openhandcursor_32.png"
        "../windows/images/openhandcursor_48.png"
        "../windows/images/openhandcursor_64.png"
        "../windows/images/splithcursor_32.png"
        "../windows/images/splithcursor_48.png"
        "../windows/images/splithcursor_64.png"
        "../windows/images/splitvcursor_32.png"
        "../windows/images/splitvcursor_48.png"
        "../windows/images/splitvcursor_64.png"
    )

    qt_internal_add_resource(QWindowsDirect2DIntegrationPlugin "cursors"
        PREFIX
            "/qt-project.org/windows/cursors"
        BASE
            "../windows"
        FILES
            ${cursors_resource_files}
    )
endif()

qt_internal_extend_target(QWindowsDirect2DIntegrationPlugin CONDITION QT_FEATURE_accessibility
    SOURCES
        ../windows/uiautomation/qwindowsuiaaccessibility.cpp ../windows/uiautomation/qwindowsuiaaccessibility.h
        ../windows/uiautomation/qwindowsuiabaseprovider.cpp ../windows/uiautomation/qwindowsuiabaseprovider.h
        ../windows/uiautomation/qwindowsuiaexpandcollapseprovider.cpp ../windows/uiautomation/qwindowsuiaexpandcollapseprovider.h
        ../windows/uiautomation/qwindowsuiagriditemprovider.cpp ../windows/uiautomation/qwindowsuiagriditemprovider.h
        ../windows/uiautomation/qwindowsuiagridprovider.cpp ../windows/uiautomation/qwindowsuiagridprovider.h
        ../windows/uiautomation/qwindowsuiainvokeprovider.cpp ../windows/uiautomation/qwindowsuiainvokeprovider.h
        ../windows/uiautomation/qwindowsuiamainprovider.cpp ../windows/uiautomation/qwindowsuiamainprovider.h
        ../windows/uiautomation/qwindowsuiaprovidercache.cpp ../windows/uiautomation/qwindowsuiaprovidercache.h
        ../windows/uiautomation/qwindowsuiarangevalueprovider.cpp ../windows/uiautomation/qwindowsuiarangevalueprovider.h
        ../windows/uiautomation/qwindowsuiaselectionitemprovider.cpp ../windows/uiautomation/qwindowsuiaselectionitemprovider.h
        ../windows/uiautomation/qwindowsuiaselectionprovider.cpp ../windows/uiautomation/qwindowsuiaselectionprovider.h
        ../windows/uiautomation/qwindowsuiatableitemprovider.cpp ../windows/uiautomation/qwindowsuiatableitemprovider.h
        ../windows/uiautomation/qwindowsuiatableprovider.cpp ../windows/uiautomation/qwindowsuiatableprovider.h
        ../windows/uiautomation/qwindowsuiatextprovider.cpp ../windows/uiautomation/qwindowsuiatextprovider.h
        ../windows/uiautomation/qwindowsuiatextrangeprovider.cpp ../windows/uiautomation/qwindowsuiatextrangeprovider.h
        ../windows/uiautomation/qwindowsuiatoggleprovider.cpp ../windows/uiautomation/qwindowsuiatoggleprovider.h
        ../windows/uiautomation/qwindowsuiautils.cpp ../windows/uiautomation/qwindowsuiautils.h
        ../windows/uiautomation/qwindowsuiavalueprovider.cpp ../windows/uiautomation/qwindowsuiavalueprovider.h
        ../windows/uiautomation/qwindowsuiawindowprovider.cpp ../windows/uiautomation/qwindowsuiawindowprovider.h
)

qt_internal_extend_target(QWindowsDirect2DIntegrationPlugin CONDITION MINGW AND QT_FEATURE_accessibility
    LIBRARIES
        uuid
)

