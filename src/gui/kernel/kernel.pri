# Qt kernel module

# Only used on platforms with CONFIG += precompile_header
PRECOMPILED_HEADER = kernel/qt_gui_pch.h


KERNEL_P= kernel
HEADERS += \
        kernel/qgenericpluginfactory_qpa.h \
        kernel/qgenericplugin_qpa.h \
        kernel/qwindowsysteminterface.h \
        kernel/qwindowsysteminterface_p.h \
        kernel/qplatformintegration.h \
        kernel/qplatformdrag.h \
        kernel/qplatformscreen.h \
        kernel/qplatformscreen_p.h \
        kernel/qplatforminputcontext.h \
        kernel/qplatforminputcontext_p.h \
        kernel/qplatforminputcontextfactory_p.h \
        kernel/qplatforminputcontextplugin_p.h \
        kernel/qplatformintegrationfactory_p.h \
        kernel/qplatformintegrationplugin.h \
        kernel/qplatformtheme.h\
        kernel/qplatformthemefactory_p.h \
        kernel/qplatformthemeplugin.h \
        kernel/qplatformwindow.h \
        kernel/qplatformcursor.h \
        kernel/qplatformclipboard.h \
        kernel/qplatformnativeinterface.h \
        kernel/qplatformmenu.h \
        kernel/qsurfaceformat.h \
        kernel/qguiapplication.h \
        kernel/qguiapplication_p.h \
        kernel/qwindow_p.h \
        kernel/qwindow.h \
        kernel/qplatformsurface.h \
        kernel/qsurface.h \
        kernel/qclipboard.h \
        kernel/qcursor.h \
        kernel/qcursor_p.h \
        kernel/qdrag.h \
        kernel/qdnd_p.h \
        kernel/qevent.h \
        kernel/qevent_p.h \
        kernel/qinputmethod.h \
        kernel/qinputmethod_p.h \
        kernel/qinputpanel.h \
        kernel/qinputpanel_p.h \
        kernel/qkeysequence.h \
        kernel/qkeysequence_p.h \
        kernel/qkeymapper_p.h \
        kernel/qpalette.h \
        kernel/qshortcutmap_p.h \
        kernel/qsessionmanager.h \
        kernel/qwindowdefs.h \
        kernel/qscreen.h \
        kernel/qscreen_p.h \
        kernel/qstylehints.h \
        kernel/qtouchdevice.h \
        kernel/qtouchdevice_p.h \
        kernel/qplatformsharedgraphicscache.h \
        kernel/qplatformdialoghelper.h \
        kernel/qplatformservices.h \
        kernel/qplatformscreenpageflipper.h

SOURCES += \
        kernel/qclipboard_qpa.cpp \
        kernel/qcursor_qpa.cpp \
        kernel/qgenericpluginfactory_qpa.cpp \
        kernel/qgenericplugin_qpa.cpp \
        kernel/qwindowsysteminterface_qpa.cpp \
        kernel/qplatforminputcontextfactory_qpa.cpp \
        kernel/qplatforminputcontextplugin_qpa.cpp \
        kernel/qplatforminputcontext_qpa.cpp \
        kernel/qplatformintegration_qpa.cpp \
        kernel/qplatformdrag_qpa.cpp \
        kernel/qplatformscreen_qpa.cpp \
        kernel/qplatformintegrationfactory_qpa.cpp \
        kernel/qplatformintegrationplugin_qpa.cpp \
        kernel/qplatformtheme_qpa.cpp \
        kernel/qplatformthemefactory_qpa.cpp \
        kernel/qplatformthemeplugin_qpa.cpp \
        kernel/qplatformwindow_qpa.cpp \
        kernel/qplatformcursor_qpa.cpp \
        kernel/qplatformclipboard_qpa.cpp \
        kernel/qplatformnativeinterface_qpa.cpp \
        kernel/qsessionmanager_qpa.cpp \
        kernel/qsurfaceformat.cpp \
        kernel/qguiapplication.cpp \
        kernel/qwindow.cpp \
        kernel/qplatformsurface_qpa.cpp \
        kernel/qsurface.cpp \
        kernel/qclipboard.cpp \
        kernel/qcursor.cpp \
        kernel/qdrag.cpp \
        kernel/qdnd.cpp \
        kernel/qevent.cpp \
        kernel/qinputmethod.cpp \
        kernel/qkeysequence.cpp \
        kernel/qkeymapper.cpp \
        kernel/qkeymapper_qpa.cpp \
        kernel/qpalette.cpp \
        kernel/qguivariant.cpp \
        kernel/qscreen.cpp \
        kernel/qshortcutmap.cpp \
        kernel/qstylehints.cpp \
        kernel/qtouchdevice.cpp \
        kernel/qplatformsharedgraphicscache_qpa.cpp \
        kernel/qplatformdialoghelper_qpa.cpp \
        kernel/qplatformservices_qpa.cpp \
        kernel/qplatformscreenpageflipper_qpa.cpp

contains(QT_CONFIG, opengl)|contains(QT_CONFIG, opengles2) {
    HEADERS += \
            kernel/qplatformopenglcontext.h \
            kernel/qopenglcontext.h \
            kernel/qopenglcontext_p.h

    SOURCES += \
            kernel/qplatformopenglcontext_qpa.cpp \
            kernel/qopenglcontext.cpp
}

win32:HEADERS+=kernel/qwindowdefs_win.h
