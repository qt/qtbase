# Qt kernel module

# Only used on platforms with CONFIG += precompile_header
PRECOMPILED_HEADER = guikernel/qt_gui_pch.h


KERNEL_P= guikernel
HEADERS += \
        guikernel/qclipboard.h \
        guikernel/qcursor.h \
        guikernel/qevent.h \
        guikernel/qevent_p.h \
        guikernel/qkeysequence.h \
        guikernel/qkeysequence_p.h \
        guikernel/qkeymapper_p.h \
        guikernel/qmime.h \
        guikernel/qsessionmanager.h \
        guikernel/qwindowdefs.h \

SOURCES += \
        guikernel/qclipboard.cpp \
        guikernel/qcursor.cpp \
        guikernel/qevent.cpp \
        guikernel/qkeysequence.cpp \
        guikernel/qkeymapper.cpp \
        guikernel/qkeymapper_qpa.cpp \
        guikernel/qmime.cpp \
        guikernel/qguivariant.cpp \

qpa {
	HEADERS += \
                guikernel/qgenericpluginfactory_qpa.h \
                guikernel/qgenericplugin_qpa.h \
                guikernel/qeventdispatcher_qpa_p.h \
                guikernel/qwindowsysteminterface_qpa.h \
                guikernel/qwindowsysteminterface_qpa_p.h \
                guikernel/qplatformintegration_qpa.h \
                guikernel/qplatformscreen_qpa.h \
                guikernel/qplatformintegrationfactory_qpa_p.h \
                guikernel/qplatformintegrationplugin_qpa.h \
                guikernel/qplatformwindow_qpa.h \
                guikernel/qplatformglcontext_qpa.h \
                guikernel/qwindowcontext_qpa.h \
                guikernel/qplatformeventloopintegration_qpa.h \
                guikernel/qplatformcursor_qpa.h \
                guikernel/qplatformclipboard_qpa.h \
                guikernel/qplatformnativeinterface_qpa.h \
                guikernel/qwindowformat_qpa.h \
                guikernel/qguiapplication.h \
                guikernel/qguiapplication_p.h \
                guikernel/qwindow_p.h \
                guikernel/qwindow.h

	SOURCES += \
                guikernel/qclipboard_qpa.cpp \
                guikernel/qcursor_qpa.cpp \
                guikernel/qgenericpluginfactory_qpa.cpp \
                guikernel/qgenericplugin_qpa.cpp \
                guikernel/qeventdispatcher_qpa.cpp \
                guikernel/qwindowsysteminterface_qpa.cpp \
                guikernel/qplatformintegration_qpa.cpp \
                guikernel/qplatformscreen_qpa.cpp \
                guikernel/qplatformintegrationfactory_qpa.cpp \
                guikernel/qplatformintegrationplugin_qpa.cpp \
                guikernel/qplatformwindow_qpa.cpp \
                guikernel/qplatformeventloopintegration_qpa.cpp \
                guikernel/qplatformglcontext_qpa.cpp \
                guikernel/qwindowcontext_qpa.cpp \
                guikernel/qplatformcursor_qpa.cpp \
                guikernel/qplatformclipboard_qpa.cpp \
                guikernel/qplatformnativeinterface_qpa.cpp \
                guikernel/qsessionmanager_qpa.cpp \
                guikernel/qwindowformat_qpa.cpp \
                guikernel/qguiapplication.cpp \
                guikernel/qwindow.cpp

        contains(QT_CONFIG, glib) {
            SOURCES += \
                guikernel/qeventdispatcher_glib_qpa.cpp
            HEADERS += \
                guikernel/qeventdispatcher_glib_qpa_p.h
            QMAKE_CXXFLAGS += $$QT_CFLAGS_GLIB
            LIBS_PRIVATE +=$$QT_LIBS_GLIB
	}
}
