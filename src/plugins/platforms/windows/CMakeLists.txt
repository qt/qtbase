# Generated from windows.pro.

#####################################################################
## qwindows Plugin:
#####################################################################

add_qt_plugin(qwindows
    TYPE platforms
    SOURCES
        openglblacklists.qrc
        qtwindowsglobal.h
        qwin10helpers.cpp qwin10helpers.h
        qwindowscombase.h
        qwindowscontext.cpp qwindowscontext.h
        qwindowscursor.cpp qwindowscursor.h
        qwindowsdialoghelpers.cpp qwindowsdialoghelpers.h
        qwindowsdropdataobject.cpp qwindowsdropdataobject.h
        qwindowsinputcontext.cpp qwindowsinputcontext.h
        qwindowsintegration.cpp qwindowsintegration.h
        qwindowsinternalmimedata.cpp qwindowsinternalmimedata.h
        qwindowskeymapper.cpp qwindowskeymapper.h
        qwindowsmenu.cpp qwindowsmenu.h
        qwindowsmime.cpp qwindowsmime.h
        qwindowsmousehandler.cpp qwindowsmousehandler.h
        qwindowsnativeinterface.cpp qwindowsnativeinterface.h
        qwindowsole.cpp qwindowsole.h
        qwindowsopengltester.cpp qwindowsopengltester.h
        qwindowspointerhandler.cpp qwindowspointerhandler.h
        qwindowsscreen.cpp qwindowsscreen.h
        qwindowsservices.cpp qwindowsservices.h
        qwindowstheme.cpp qwindowstheme.h
        qwindowsthreadpoolrunner.h
        qwindowswindow.cpp qwindowswindow.h
        main.cpp
        qwindowsbackingstore.cpp qwindowsbackingstore.h
        qwindowsgdiintegration.cpp qwindowsgdiintegration.h
        qwindowsgdinativeinterface.cpp qwindowsgdinativeinterface.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_FOREACH
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::EventDispatcherSupportPrivate
        Qt::FontDatabaseSupportPrivate
        Qt::ThemeSupportPrivate
        gdi32
        dwmapi
        ole32
        user32
        winspool
        imm32
        winmm
        oleaut32
        shlwapi
        shell32
        advapi32
        wtsapi32
    # OTHER_FILES = "windows.json"
    # PLUGIN_CLASS_NAME = "QWindowsIntegrationPlugin"
    # QT_FOR_CONFIG = "gui"
    # _LOADED = "qt_plugin"
)

## Scopes:
#####################################################################

# TODO
extend_target(qwindows CONDITION QT_FEATURE_accessibility
    SOURCES
        uiautomation/qwindowsuiaaccessibility.cpp uiautomation/qwindowsuiaaccessibility.h
        uiautomation/qwindowsuiabaseprovider.cpp uiautomation/qwindowsuiabaseprovider.h
        uiautomation/qwindowsuiagriditemprovider.cpp uiautomation/qwindowsuiagriditemprovider.h
        uiautomation/qwindowsuiagridprovider.cpp uiautomation/qwindowsuiagridprovider.h
        uiautomation/qwindowsuiainvokeprovider.cpp uiautomation/qwindowsuiainvokeprovider.h
        uiautomation/qwindowsuiamainprovider.cpp uiautomation/qwindowsuiamainprovider.h
        uiautomation/qwindowsuiaprovidercache.cpp uiautomation/qwindowsuiaprovidercache.h
        uiautomation/qwindowsuiarangevalueprovider.cpp uiautomation/qwindowsuiarangevalueprovider.h
        uiautomation/qwindowsuiaselectionitemprovider.cpp uiautomation/qwindowsuiaselectionitemprovider.h
        uiautomation/qwindowsuiaselectionprovider.cpp uiautomation/qwindowsuiaselectionprovider.h
        uiautomation/qwindowsuiatableitemprovider.cpp uiautomation/qwindowsuiatableitemprovider.h
        uiautomation/qwindowsuiatableprovider.cpp uiautomation/qwindowsuiatableprovider.h
        uiautomation/qwindowsuiatextprovider.cpp uiautomation/qwindowsuiatextprovider.h
        uiautomation/qwindowsuiatextrangeprovider.cpp uiautomation/qwindowsuiatextrangeprovider.h
        uiautomation/qwindowsuiatoggleprovider.cpp uiautomation/qwindowsuiatoggleprovider.h
        uiautomation/qwindowsuiautils.cpp uiautomation/qwindowsuiautils.h
        uiautomation/qwindowsuiavalueprovider.cpp uiautomation/qwindowsuiavalueprovider.h
    LIBRARIES
        Qt::AccessibilitySupportPrivate
)

# Special case, disable vulkan for now because headers can't be found.
extend_target(qwindows CONDITION QT_FEATURE_vulkan
    SOURCES
        qwindowsvulkaninstance.cpp qwindowsvulkaninstance.h
    LIBRARIES
        vulkan_supportPrivate
)

#extend_target(qwindows CONDITION NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN
#    # PLUGIN_EXTENDS = "-"
#)

extend_target(qwindows CONDITION QT_FEATURE_opengl AND NOT QT_FEATURE_dynamicgl AND NOT QT_FEATURE_opengles2
    LIBRARIES
        opengl32
)

extend_target(qwindows CONDITION mingw
    LIBRARIES
        uuid
)

extend_target(qwindows CONDITION QT_FEATURE_opengl
    SOURCES
        qwindowsopenglcontext.h
)

extend_target(qwindows CONDITION QT_FEATURE_opengles2
    SOURCES
        qwindowseglcontext.cpp qwindowseglcontext.h
)

extend_target(qwindows CONDITION QT_FEATURE_opengl AND NOT QT_FEATURE_opengles2
    SOURCES
        qwindowsglcontext.cpp qwindowsglcontext.h
)

extend_target(qwindows CONDITION QT_FEATURE_opengl_dynamic
    SOURCES
        qwindowseglcontext.cpp qwindowseglcontext.h
)

extend_target(qwindows CONDITION QT_FEATURE_systemtrayicon
    SOURCES
        qwindowssystemtrayicon.cpp qwindowssystemtrayicon.h
)

extend_target(qwindows CONDITION QT_FEATURE_clipboard
    SOURCES
        qwindowsclipboard.cpp qwindowsclipboard.h
)

extend_target(qwindows CONDITION QT_FEATURE_clipboard AND QT_FEATURE_draganddrop
    SOURCES
        qwindowsdrag.cpp qwindowsdrag.h
)

extend_target(qwindows CONDITION QT_FEATURE_tabletevent
    SOURCES
        qwindowstabletsupport.cpp qwindowstabletsupport.h
    INCLUDE_DIRECTORIES
        ${PROJECT_SOURCE_DIR}/src/3rdparty/wintab
)

extend_target(qwindows CONDITION QT_FEATURE_sessionmanager
    SOURCES
        qwindowssessionmanager.cpp qwindowssessionmanager.h
)

extend_target(qwindows CONDITION QT_FEATURE_imageformat_png
    SOURCES
        cursors.qrc
)

# TODO
extend_target(qwindows CONDITION (QT_FEATURE_accessibility)
    LIBRARIES
        Qt::WindowsUIAutomationSupportPrivate
)

extend_target(qwindows CONDITION QT_FEATURE_accessibility AND mingw
    LIBRARIES
        uuid
)

#extend_target(qwindows CONDITION QT_FEATURE_combined_angle_lib
#    DEFINES
#        LIBEGL_NAME=
#        LIBGLESV2_NAME=
#)

#extend_target(qwindows CONDITION NOT QT_FEATURE_combined_angle_lib
#    DEFINES
#        LIBEGL_NAME=
#        LIBGLESV2_NAME=
#)
