# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QAndroidIntegrationPlugin Plugin:
#####################################################################
qt_find_package(EGL MODULE)

qt_internal_add_plugin(QAndroidIntegrationPlugin
    OUTPUT_NAME qtforandroid
    PLUGIN_TYPE platforms
    DEFAULT_IF "android" IN_LIST QT_QPA_PLATFORMS
    SOURCES
        androidcontentfileengine.cpp androidcontentfileengine.h
        qandroidapkfileengine.h qandroidapkfileengine.cpp
        androidjniinput.cpp androidjniinput.h
        androidjnimain.cpp androidjnimain.h
        androidjnimenu.cpp androidjnimenu.h
        main.cpp
        qandroidassetsfileenginehandler.cpp qandroidassetsfileenginehandler.h
        qandroideventdispatcher.cpp qandroideventdispatcher.h
        qandroidinputcontext.cpp qandroidinputcontext.h
        qandroidplatformdialoghelpers.cpp qandroidplatformdialoghelpers.h
        qandroidplatformfiledialoghelper.cpp qandroidplatformfiledialoghelper.h
        qandroidplatformfontdatabase.cpp qandroidplatformfontdatabase.h
        qandroidplatformforeignwindow.cpp qandroidplatformforeignwindow.h
        qandroidplatformiconengine.cpp qandroidplatformiconengine.h
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
        androidwindowembedding.cpp androidwindowembedding.h
        androidbackendregister.cpp androidbackendregister.h
    NO_UNITY_BUILD_SOURCES
        # Conflicting symbols and macros with androidjnimain.cpp
        # TODO: Unify the usage of FIND_AND_CHECK_CLASS, and similar
        #       macros. Q_JNI_FIND_AND_CHECK_CLASS in `qjnihelpers_p.h`
        #       seems to be doing most of the work already.
        androidjnimenu.cpp
        qandroidinputcontext.cpp
        qandroidplatformdialoghelpers.cpp
        # Conflicting JNI classes, and types
        androidcontentfileengine.cpp
        qandroidplatformforeignwindow.cpp
        qandroidplatformintegration.cpp
        qandroidplatformscreen.cpp
        qandroidplatformservices.cpp
        qandroidplatformwindow.cpp
        qandroidsystemlocale.cpp
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

qt_internal_extend_target(QAndroidIntegrationPlugin CONDITION QT_FEATURE_clipboard
    SOURCES
        qandroidplatformclipboard.cpp qandroidplatformclipboard.h
)

qt_internal_extend_target(QAndroidIntegrationPlugin CONDITION QT_FEATURE_accessibility
    SOURCES
        androidjniaccessibility.cpp androidjniaccessibility.h
        qandroidplatformaccessibility.cpp qandroidplatformaccessibility.h
    NO_UNITY_BUILD_SOURCES
        androidjniaccessibility.cpp
)

qt_internal_extend_target(QAndroidIntegrationPlugin CONDITION QT_FEATURE_vulkan
    SOURCES
        qandroidplatformvulkaninstance.cpp qandroidplatformvulkaninstance.h
        qandroidplatformvulkanwindow.cpp qandroidplatformvulkanwindow.h
    NO_UNITY_BUILD_SOURCES
        # To avoid undefined symbols due to missing VK_USE_PLATFORM_ANDROID_KHR
        qandroidplatformvulkaninstance.cpp qandroidplatformvulkanwindow.cpp
)
