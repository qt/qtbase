# Qt kernel module

# Only used on platforms with CONFIG += precompile_header
PRECOMPILED_HEADER = kernel/qt_gui_pch.h


KERNEL_P= kernel
HEADERS += \
        kernel/qtguiglobal.h \
        kernel/qtguiglobal_p.h \
        kernel/qgenericpluginfactory.h \
        kernel/qgenericplugin.h \
        kernel/qwindowsysteminterface.h \
        kernel/qwindowsysteminterface_p.h \
        kernel/qplatformintegration.h \
        kernel/qplatformscreen.h \
        kernel/qplatformscreen_p.h \
        kernel/qplatforminputcontext.h \
        kernel/qplatforminputcontext_p.h \
        kernel/qplatforminputcontextfactory_p.h \
        kernel/qplatforminputcontextplugin_p.h \
        kernel/qplatformintegrationfactory_p.h \
        kernel/qplatformintegrationplugin.h \
        kernel/qplatformtheme.h\
        kernel/qplatformtheme_p.h \
        kernel/qplatformthemefactory_p.h \
        kernel/qplatformthemeplugin.h \
        kernel/qplatformwindow.h \
        kernel/qplatformoffscreensurface.h \
        kernel/qplatformwindow_p.h \
        kernel/qplatformcursor.h \
        kernel/qplatformclipboard.h \
        kernel/qplatformnativeinterface.h \
        kernel/qplatformmenu.h \
        kernel/qsurfaceformat.h \
        kernel/qguiapplication.h \
        kernel/qguiapplication_p.h \
        kernel/qwindow_p.h \
        kernel/qwindow.h \
        kernel/qoffscreensurface.h \
        kernel/qplatformsurface.h \
        kernel/qsurface.h \
        kernel/qclipboard.h \
        kernel/qcursor.h \
        kernel/qcursor_p.h \
        kernel/qevent.h \
        kernel/qevent_p.h \
        kernel/qinputmethod.h \
        kernel/qinputmethod_p.h \
        kernel/qinternalmimedata_p.h \
        kernel/qkeysequence.h \
        kernel/qkeysequence_p.h \
        kernel/qkeymapper_p.h \
        kernel/qpalette.h \
        kernel/qshortcutmap_p.h \
        kernel/qsessionmanager.h \
        kernel/qsessionmanager_p.h \
        kernel/qwindowdefs.h \
        kernel/qscreen.h \
        kernel/qscreen_p.h \
        kernel/qstylehints.h \
        kernel/qtouchdevice.h \
        kernel/qtouchdevice_p.h \
        kernel/qplatformsharedgraphicscache.h \
        kernel/qplatformdialoghelper.h \
        kernel/qplatformservices.h \
        kernel/qplatformsystemtrayicon.h \
        kernel/qplatformsessionmanager.h \
        kernel/qpixelformat.h \
        kernel/qpaintdevicewindow.h \
        kernel/qpaintdevicewindow_p.h \
        kernel/qrasterwindow.h \
        kernel/qplatformgraphicsbuffer.h \
        kernel/qplatformgraphicsbufferhelper.h \
        kernel/qinputdevicemanager_p.h \
        kernel/qinputdevicemanager_p_p.h \
        kernel/qhighdpiscaling_p.h


SOURCES += \
        kernel/qgenericpluginfactory.cpp \
        kernel/qgenericplugin.cpp \
        kernel/qwindowsysteminterface.cpp \
        kernel/qplatforminputcontextfactory.cpp \
        kernel/qplatforminputcontextplugin.cpp \
        kernel/qplatforminputcontext.cpp \
        kernel/qplatformintegration.cpp \
        kernel/qplatformscreen.cpp \
        kernel/qplatformintegrationfactory.cpp \
        kernel/qplatformintegrationplugin.cpp \
        kernel/qplatformtheme.cpp \
        kernel/qplatformthemefactory.cpp \
        kernel/qplatformthemeplugin.cpp \
        kernel/qplatformwindow.cpp \
        kernel/qplatformoffscreensurface.cpp \
        kernel/qplatformcursor.cpp \
        kernel/qplatformclipboard.cpp \
        kernel/qplatformnativeinterface.cpp \
        kernel/qsessionmanager.cpp \
        kernel/qsurfaceformat.cpp \
        kernel/qguiapplication.cpp \
        kernel/qwindow.cpp \
        kernel/qoffscreensurface.cpp \
        kernel/qplatformsurface.cpp \
        kernel/qsurface.cpp \
        kernel/qclipboard.cpp \
        kernel/qcursor.cpp \
        kernel/qevent.cpp \
        kernel/qinputmethod.cpp \
        kernel/qinternalmimedata.cpp \
        kernel/qkeysequence.cpp \
        kernel/qkeymapper.cpp \
        kernel/qpalette.cpp \
        kernel/qguivariant.cpp \
        kernel/qscreen.cpp \
        kernel/qshortcutmap.cpp \
        kernel/qstylehints.cpp \
        kernel/qtouchdevice.cpp \
        kernel/qplatformsharedgraphicscache.cpp \
        kernel/qplatformdialoghelper.cpp \
        kernel/qplatformservices.cpp \
        kernel/qplatformsystemtrayicon.cpp \
        kernel/qplatformsessionmanager.cpp \
        kernel/qplatformmenu.cpp \
        kernel/qpixelformat.cpp \
        kernel/qpaintdevicewindow.cpp \
        kernel/qrasterwindow.cpp \
        kernel/qplatformgraphicsbuffer.cpp \
        kernel/qplatformgraphicsbufferhelper.cpp \
        kernel/qinputdevicemanager.cpp \
        kernel/qhighdpiscaling.cpp

qtConfig(draganddrop) {
    HEADERS += \
        kernel/qdnd_p.h \
        kernel/qdrag.h \
        kernel/qplatformdrag.h \
        kernel/qshapedpixmapdndwindow_p.h \
        kernel/qsimpledrag_p.h

    SOURCES += \
        kernel/qdnd.cpp \
        kernel/qdrag.cpp \
        kernel/qplatformdrag.cpp \
        kernel/qshapedpixmapdndwindow.cpp \
        kernel/qsimpledrag.cpp
}

qtConfig(opengl) {
    HEADERS += \
            kernel/qplatformopenglcontext.h \
            kernel/qopenglcontext.h \
            kernel/qopenglcontext_p.h \
            kernel/qopenglwindow.h

    SOURCES += \
            kernel/qplatformopenglcontext.cpp \
            kernel/qopenglcontext.cpp \
            kernel/qopenglwindow.cpp
}

win32:HEADERS+=kernel/qwindowdefs_win.h
