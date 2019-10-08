# Generated from xcb_glx.pro.

#####################################################################
## qxcb-glx-integration Plugin:
#####################################################################

add_qt_plugin(qxcb-glx-integration
    TYPE xcbglintegrations
    CLASS_NAME QXcbGlxIntegrationPlugin
    SOURCES
        qglxintegration.cpp qglxintegration.h
        qxcbglxintegration.cpp qxcbglxintegration.h
        qxcbglxmain.cpp
        qxcbglxnativeinterfacehandler.cpp qxcbglxnativeinterfacehandler.h
        qxcbglxwindow.cpp qxcbglxwindow.h
    DEFINES
        QT_NO_FOREACH
    INCLUDE_DIRECTORIES
        ..
        ../..
    LIBRARIES
        Qt::CorePrivate
        Qt::GlxSupportPrivate
        Qt::GuiPrivate
        Qt::XcbQpaPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::GlxSupport
        Qt::Gui
        Qt::XcbQpa
)

#### Keys ignored in scope 1:.:.:xcb_glx.pro:<TRUE>:
# _LOADED = "qt_plugin" "qt_build_paths"

## Scopes:
#####################################################################

extend_target(qxcb-glx-integration CONDITION QT_FEATURE_xcb_glx
    PUBLIC_LIBRARIES
        XCB::GLX
)

extend_target(qxcb-glx-integration CONDITION QT_FEATURE_dlopen AND NOT static
    PUBLIC_LIBRARIES
        ${CMAKE_DL_LIBS}
)
