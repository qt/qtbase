# Generated from xcb_egl.pro.

#####################################################################
## qxcb-egl-integration Plugin:
#####################################################################

add_qt_plugin(qxcb-egl-integration
    TYPE xcbglintegrations
    CLASS_NAME QXcbEglIntegrationPlugin
    SOURCES
        qxcbeglcontext.h
        qxcbeglintegration.cpp qxcbeglintegration.h
        qxcbeglmain.cpp
        qxcbeglnativeinterfacehandler.cpp qxcbeglnativeinterfacehandler.h
        qxcbeglwindow.cpp qxcbeglwindow.h
    DEFINES
        QT_NO_FOREACH
    INCLUDE_DIRECTORIES
        ..
        ../..
    LIBRARIES
        Qt::CorePrivate
        Qt::EglSupportPrivate
        Qt::GuiPrivate
        Qt::XcbQpaPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::EglSupport
        Qt::Gui
        Qt::XcbQpa
)

#### Keys ignored in scope 1:.:.:xcb_egl.pro:<TRUE>:
# _LOADED = "qt_plugin" "qt_build_paths"
