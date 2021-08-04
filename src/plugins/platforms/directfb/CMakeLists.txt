# Generated from directfb.pro.

# begin special case:
qt_find_package(DirectFB)
qt_find_package(EGL)
# end special case:

#####################################################################
## QDirectFbIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QDirectFbIntegrationPlugin
    OUTPUT_NAME qdirectfb
    PLUGIN_TYPE platforms
    SOURCES
        main.cpp
        qdirectfbbackingstore.cpp qdirectfbbackingstore.h
        qdirectfbblitter.cpp qdirectfbblitter.h
        qdirectfbconvenience.cpp qdirectfbconvenience.h
        qdirectfbcursor.cpp qdirectfbcursor.h
        qdirectfbeglhooks.h
        qdirectfbinput.cpp qdirectfbinput.h
        qdirectfbintegration.cpp qdirectfbintegration.h
        qdirectfbscreen.cpp qdirectfbscreen.h
        qdirectfbwindow.cpp qdirectfbwindow.h
    LIBRARIES
        PkgConfig::DirectFB
        EGL::EGL # special case
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:.:directfb.pro:<TRUE>:
# OTHER_FILES = "directfb.json"

## Scopes:
#####################################################################

qt_internal_extend_target(QDirectFbIntegrationPlugin CONDITION QT_CONFIG___contains___directfb_egl
    SOURCES
        qdirectfb_egl.cpp qdirectfb_egl.h
    DEFINES
        DIRECTFB_GL_EGL
)

qt_internal_extend_target(QDirectFbIntegrationPlugin CONDITION NOT DIRECTFB_PLATFORM_HOOKS_SOURCES_ISEMPTY
    DEFINES
        DIRECTFB_PLATFORM_HOOKS
)

#### Keys ignored in scope 3:.:.:directfb.pro:NOT DIRECTFB_PLATFORM_HOOKS_SOURCES_ISEMPTY:
# QMAKE_LIBDIR = "$$DIRECTFB_PLATFORM_HOOKS_LIBDIR"

qt_internal_extend_target(QDirectFbIntegrationPlugin CONDITION DIRECTFB_PLATFORM_HOOKS_SOURCES_ISEMPTY
    SOURCES
        qdirectfbeglhooks_stub.cpp
)

#### Keys ignored in scope 5:.:.:directfb.pro:NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN:
# PLUGIN_EXTENDS = "-"
