# Generated from xcb_egl.pro.

#####################################################################
## QXcbEglIntegrationPlugin Plugin:
#####################################################################

qt_find_package(EGL) # special case

qt_internal_add_plugin(QXcbEglIntegrationPlugin
    OUTPUT_NAME qxcb-egl-integration
    PLUGIN_TYPE xcbglintegrations
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
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::XcbQpaPrivate
        EGL::EGL # special case
)
