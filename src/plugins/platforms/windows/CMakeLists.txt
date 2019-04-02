# Generated from windows.pro.

#####################################################################
## qwindows Plugin:
#####################################################################

add_qt_plugin(qwindows
    TYPE platforms
    SOURCES
        main.cpp
        qtwindowsglobal.h
        qwin10helpers.cpp qwin10helpers.h
        qwindowsbackingstore.cpp qwindowsbackingstore.h
        qwindowscombase.h
        qwindowscontext.cpp qwindowscontext.h
        qwindowscursor.cpp qwindowscursor.h
        qwindowsdialoghelpers.cpp qwindowsdialoghelpers.h
        qwindowsdropdataobject.cpp qwindowsdropdataobject.h
        qwindowsgdiintegration.cpp qwindowsgdiintegration.h
        qwindowsgdinativeinterface.cpp qwindowsgdinativeinterface.h
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
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_FOREACH
    LIBRARIES
        Qt::CorePrivate
        Qt::EventDispatcherSupportPrivate
        Qt::FontDatabaseSupportPrivate
        Qt::GuiPrivate
        Qt::ThemeSupportPrivate
        advapi32
        dwmapi
        gdi32
        imm32
        ole32
        oleaut32
        shell32
        shlwapi
        user32
        winmm
        winspool
        wtsapi32
)

# Resources:
set_source_files_properties("openglblacklists/default.json"
    PROPERTIES alias "default.json")
add_qt_resource(qwindows "openglblacklists" PREFIX "/qt-project.org/windows/openglblacklists" FILES
    openglblacklists/default.json)


#### Keys ignored in scope 1:.:windows.pro:<NONE>:
# OTHER_FILES = "windows.json"
# PLUGIN_CLASS_NAME = "QWindowsIntegrationPlugin"
# QT_FOR_CONFIG = "gui"
# _LOADED = "qt_plugin"

## Scopes:
#####################################################################

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

extend_target(qwindows CONDITION QT_FEATURE_vulkan
    SOURCES
        qwindowsvulkaninstance.cpp qwindowsvulkaninstance.h
    LIBRARIES
        Qt::VulkanSupportPrivate
)

#### Keys ignored in scope 4:.:windows.pro:NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN:
# PLUGIN_EXTENDS = "-"

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

extend_target(qwindows CONDITION QT_FEATURE_dynamicgl
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

# Resources:
add_qt_resource(qwindows "cursors" PREFIX "/qt-project.org/windows/cursors" FILES
    images/closedhandcursor_32.png
    images/closedhandcursor_48.png
    images/closedhandcursor_64.png
    images/dragcopycursor_32.png
    images/dragcopycursor_48.png
    images/dragcopycursor_64.png
    images/draglinkcursor_32.png
    images/draglinkcursor_48.png
    images/draglinkcursor_64.png
    images/dragmovecursor_32.png
    images/dragmovecursor_48.png
    images/dragmovecursor_64.png
    images/openhandcursor_32.png
    images/openhandcursor_48.png
    images/openhandcursor_64.png
    images/splithcursor_32.png
    images/splithcursor_48.png
    images/splithcursor_64.png
    images/splitvcursor_32.png
    images/splitvcursor_48.png
    images/splitvcursor_64.png)


extend_target(qwindows CONDITION (QT_FEATURE_accessibility) AND (TARGET windowsuiautomation_supportPrivate)
    LIBRARIES
        windowsuiautomation_supportPrivate
)

extend_target(qwindows CONDITION QT_FEATURE_accessibility AND mingw
    LIBRARIES
        uuid
)

# special case:
# extend_target(qwindows CONDITION QT_FEATURE_combined_angle_lib ...
# extend_target(qwindows CONDITION NOT QT_FEATURE_combined_angle_lib ...
