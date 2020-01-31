# Generated from xcb_egl.pro.

#####################################################################
## QXcbEglIntegrationPlugin Plugin:
#####################################################################

qt_add_plugin(QXcbEglIntegrationPlugin
    OUTPUT_NAME qxcb-egl-integration
    TYPE xcbglintegrations
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
