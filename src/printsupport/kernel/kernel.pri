HEADERS += \
        $$PWD/qpaintengine_alpha_p.h \
        $$PWD/qpaintengine_preview_p.h \
        $$PWD/qprintengine.h \
        $$PWD/qprinter.h \
        $$PWD/qprinter_p.h \
        $$PWD/qprinterinfo.h \
        $$PWD/qprinterinfo_p.h \
        $$PWD/qplatformprintplugin_qpa.h \
        $$PWD/qplatformprintersupport_qpa.h

SOURCES += \
        $$PWD/qpaintengine_alpha.cpp \
        $$PWD/qpaintengine_preview.cpp \
        $$PWD/qprintengine_pdf.cpp \
        $$PWD/qprinter.cpp \
        $$PWD/qprinterinfo.cpp \
        $$PWD/qplatformprintplugin.cpp \
        $$PWD/qplatformprintersupport_qpa.cpp

unix:!symbian {
        HEADERS += \
                $$PWD/qprinterinfo_unix_p.h
        SOURCES += \
                $$PWD/qprinterinfo_unix.cpp
}

win32 {
        HEADERS += \
                $$PWD/qprintengine_win_p.h
        SOURCES += \
                $$PWD/qprintengine_win.cpp
        LIBS += -lWinspool -lComdlg32
}

x11|qpa:!win32 {
        SOURCES += $$PWD/qcups.cpp
        HEADERS += $$PWD/qcups_p.h
} else {
        DEFINES += QT_NO_CUPS QT_NO_LPR
}
