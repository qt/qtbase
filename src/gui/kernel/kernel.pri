# Qt kernel module

# Only used on platforms with CONFIG += precompile_header
PRECOMPILED_HEADER = kernel/qt_gui_pch.h


KERNEL_P= kernel
HEADERS += \
        kernel/qclipboard.h \
        kernel/qcursor.h \
        kernel/qcursor_p.h \
        kernel/qdrag.h \
        kernel/qdnd_p.h \
        kernel/qevent.h \
        kernel/qevent_p.h \
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
        kernel/qstylehints.h

SOURCES += \
        kernel/qclipboard.cpp \
        kernel/qcursor.cpp \
        kernel/qdrag.cpp \
        kernel/qdnd.cpp \
        kernel/qevent.cpp \
        kernel/qinputpanel.cpp \
        kernel/qkeysequence.cpp \
        kernel/qkeymapper.cpp \
        kernel/qkeymapper_qpa.cpp \
        kernel/qpalette.cpp \
        kernel/qguivariant.cpp \
        kernel/qscreen.cpp \
        kernel/qshortcutmap.cpp \
        kernel/qstylehints.cpp

qpa {
	HEADERS += \
                kernel/qgenericpluginfactory_qpa.h \
                kernel/qgenericplugin_qpa.h \
                kernel/qwindowsysteminterface_qpa.h \
                kernel/qwindowsysteminterface_qpa_p.h \
                kernel/qplatformintegration_qpa.h \
                kernel/qplatformdrag_qpa.h \
                kernel/qplatformscreen_qpa.h \
                kernel/qplatforminputcontext_qpa.h \
                kernel/qplatformintegrationfactory_qpa_p.h \
                kernel/qplatformintegrationplugin_qpa.h \
                kernel/qplatformwindow_qpa.h \
                kernel/qplatformopenglcontext_qpa.h \
                kernel/qopenglcontext.h \
                kernel/qopenglcontext_p.h \
                kernel/qplatformcursor_qpa.h \
                kernel/qplatformclipboard_qpa.h \
                kernel/qplatformnativeinterface_qpa.h \
                kernel/qsurfaceformat.h \
                kernel/qguiapplication.h \
                kernel/qguiapplication_p.h \
                kernel/qwindow_p.h \
                kernel/qwindow.h \
                kernel/qplatformsurface_qpa.h \
                kernel/qsurface.h

	SOURCES += \
                kernel/qclipboard_qpa.cpp \
                kernel/qcursor_qpa.cpp \
                kernel/qgenericpluginfactory_qpa.cpp \
                kernel/qgenericplugin_qpa.cpp \
                kernel/qwindowsysteminterface_qpa.cpp \
                kernel/qplatforminputcontext_qpa.cpp \
                kernel/qplatformintegration_qpa.cpp \
                kernel/qplatformscreen_qpa.cpp \
                kernel/qplatformintegrationfactory_qpa.cpp \
                kernel/qplatformintegrationplugin_qpa.cpp \
                kernel/qplatformwindow_qpa.cpp \
                kernel/qplatformopenglcontext_qpa.cpp \
                kernel/qopenglcontext.cpp \
                kernel/qplatformcursor_qpa.cpp \
                kernel/qplatformclipboard_qpa.cpp \
                kernel/qplatformnativeinterface_qpa.cpp \
                kernel/qsessionmanager_qpa.cpp \
                kernel/qsurfaceformat.cpp \
                kernel/qguiapplication.cpp \
                kernel/qwindow.cpp \
                kernel/qplatformsurface_qpa.cpp \
                kernel/qsurface.cpp
}

win32:HEADERS+=kernel/qwindowdefs_win.h
