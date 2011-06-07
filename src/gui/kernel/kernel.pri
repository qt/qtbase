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
        kernel/qkeysequence.h \
        kernel/qkeysequence_p.h \
        kernel/qkeymapper_p.h \
        kernel/qmime.h \
        kernel/qpalette.h \
        kernel/qsessionmanager.h \
        kernel/qwindowdefs.h \

SOURCES += \
        kernel/qclipboard.cpp \
        kernel/qcursor.cpp \
        kernel/qdrag.cpp \
        kernel/qdnd.cpp \
        kernel/qevent.cpp \
        kernel/qkeysequence.cpp \
        kernel/qkeymapper.cpp \
        kernel/qkeymapper_qpa.cpp \
        kernel/qmime.cpp \
        kernel/qpalette.cpp \
        kernel/qguivariant.cpp \

qpa {
	HEADERS += \
                kernel/qgenericpluginfactory_qpa.h \
                kernel/qgenericplugin_qpa.h \
                kernel/qeventdispatcher_qpa_p.h \
                kernel/qwindowsysteminterface_qpa.h \
                kernel/qwindowsysteminterface_qpa_p.h \
                kernel/qplatformintegration_qpa.h \
                kernel/qplatformdrag_qpa.h \
                kernel/qplatformscreen_qpa.h \
                kernel/qplatformintegrationfactory_qpa_p.h \
                kernel/qplatformintegrationplugin_qpa.h \
                kernel/qplatformwindow_qpa.h \
                kernel/qplatformglcontext_qpa.h \
                kernel/qguiglcontext_qpa.h \
                kernel/qplatformcursor_qpa.h \
                kernel/qplatformclipboard_qpa.h \
                kernel/qplatformnativeinterface_qpa.h \
                kernel/qguiglformat_qpa.h \
                kernel/qguiapplication.h \
                kernel/qguiapplication_p.h \
                kernel/qwindow_p.h \
                kernel/qwindow.h

	SOURCES += \
                kernel/qclipboard_qpa.cpp \
                kernel/qcursor_qpa.cpp \
                kernel/qgenericpluginfactory_qpa.cpp \
                kernel/qgenericplugin_qpa.cpp \
                kernel/qeventdispatcher_qpa.cpp \
                kernel/qwindowsysteminterface_qpa.cpp \
                kernel/qplatformintegration_qpa.cpp \
                kernel/qplatformscreen_qpa.cpp \
                kernel/qplatformintegrationfactory_qpa.cpp \
                kernel/qplatformintegrationplugin_qpa.cpp \
                kernel/qplatformwindow_qpa.cpp \
                kernel/qplatformglcontext_qpa.cpp \
                kernel/qguiglcontext_qpa.cpp \
                kernel/qplatformcursor_qpa.cpp \
                kernel/qplatformclipboard_qpa.cpp \
                kernel/qplatformnativeinterface_qpa.cpp \
                kernel/qsessionmanager_qpa.cpp \
                kernel/qguiglformat_qpa.cpp \
                kernel/qguiapplication.cpp \
                kernel/qwindow.cpp

        contains(QT_CONFIG, glib) {
            SOURCES += \
                kernel/qeventdispatcher_glib_qpa.cpp
            HEADERS += \
                kernel/qeventdispatcher_glib_qpa_p.h
            QMAKE_CXXFLAGS += $$QT_CFLAGS_GLIB
            LIBS_PRIVATE +=$$QT_LIBS_GLIB
	}
}

mac {
    HEADERS += \
        kernel/qeventdispatcher_mac_p.h
    OBJECTIVE_SOURCES += \
        kernel/qeventdispatcher_mac.mm
    LIBS += -framework CoreFoundation -framework Cocoa -framework Carbon
}

win32:HEADERS+=kernel/qwindowdefs_win.h
