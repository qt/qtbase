# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QAndroidIntegrationPlugin Plugin:
#####################################################################
qt_find_package(EGL)

qt_internal_add_plugin(QAndroidIntegrationPlugin
    OUTPUT_NAME qtforandroid
    PLUGIN_TYPE platforms
    DEFAULT_IF ${QT_QPA_DEFAULT_PLATFORM} MATCHES android
    SOURCES
        androidcontentfileengine.cpp androidcontentfileengine.h
        androiddeadlockprotector.h
        androidjniaccessibility.cpp androidjniaccessibility.h
        androidjniclipboard.cpp androidjniclipboard.h
        androidjniinput.cpp androidjniinput.h
        androidjnimain.cpp androidjnimain.h
        androidjnimenu.cpp androidjnimenu.h
        androidsurfaceclient.h
        main.cpp
        qandroidassetsfileenginehandler.cpp qandroidassetsfileenginehandler.h
        qandroideventdispatcher.cpp qandroideventdispatcher.h
        qandroidinputcontext.cpp qandroidinputcontext.h
        qandroidplatformaccessibility.cpp qandroidplatformaccessibility.h
        qandroidplatformbackingstore.cpp qandroidplatformbackingstore.h
        qandroidplatformclipboard.cpp qandroidplatformclipboard.h
        qandroidplatformdialoghelpers.cpp qandroidplatformdialoghelpers.h
        qandroidplatformfiledialoghelper.cpp qandroidplatformfiledialoghelper.h
        qandroidplatformfontdatabase.cpp qandroidplatformfontdatabase.h
        qandroidplatformforeignwindow.cpp qandroidplatformforeignwindow.h
        qandroidplatformintegration.cpp qandroidplatformintegration.h
        qandroidplatformmenu.cpp qandroidplatformmenu.h
        qandroidplatformmenubar.cpp qandroidplatformmenubar.h
        qandroidplatformmenuitem.cpp qandroidplatformmenuitem.h
        qandroidplatformoffscreensurface.cpp qandroidplatformoffscreensurface.h
        qandroidplatformopenglcontext.cpp qandroidplatformopenglcontext.h
        qandroidplatformopenglwindow.cpp qandroidplatformopenglwindow.h
        qandroidplatformscreen.cpp qandroidplatformscreen.h
        qandroidplatformservices.cpp qandroidplatformservices.h
        qandroidplatformtheme.cpp qandroidplatformtheme.h
        qandroidplatformwindow.cpp qandroidplatformwindow.h
        qandroidsystemlocale.cpp qandroidsystemlocale.h
    DEFINES
        QT_USE_QSTRINGBUILDER
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${QtBase_SOURCE_DIR}/src/3rdparty/android
    LIBRARIES
        EGL::EGL
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        android
        jnigraphics
        EGL::EGL
)

## Scopes:
#####################################################################

qt_internal_extend_target(QAndroidIntegrationPlugin CONDITION QT_FEATURE_android_style_assets
    SOURCES
        extract.cpp
)

qt_internal_extend_target(QAndroidIntegrationPlugin CONDITION NOT QT_FEATURE_android_style_assets
    SOURCES
        extract-dummy.cpp
)

qt_internal_extend_target(QAndroidIntegrationPlugin CONDITION QT_FEATURE_vulkan
    SOURCES
        qandroidplatformvulkaninstance.cpp qandroidplatformvulkaninstance.h
        qandroidplatformvulkanwindow.cpp qandroidplatformvulkanwindow.h
)
