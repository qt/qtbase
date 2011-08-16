HEADERS += \
        $$PWD/qpaintengine_alpha_p.h \
        $$PWD/qpaintengine_preview_p.h \
        $$PWD/qprintengine.h \
        $$PWD/qprinter.h \
        $$PWD/qprinter_p.h \
        $$PWD/qprinterinfo.h \
        $$PWD/qprinterinfo_p.h

SOURCES += \
        $$PWD/qpaintengine_alpha.cpp \
        $$PWD/qpaintengine_preview.cpp \
        $$PWD/qprintengine_pdf.cpp \
        $$PWD/qprinter.cpp \
        $$PWD/qprinterinfo.cpp \

unix:!symbian {
        HEADERS += \
                $$PWD/qprinterinfo_unix_p.h
        SOURCES += \
                $$PWD/qprinterinfo_unix.cpp
}

qpa {
        HEADERS += $$PWD/qplatformprintersupport_qpa.h
        SOURCES += \
                $$PWD/qplatformprintersupport_qpa.cpp
}


x11|qpa:!win32 {
        SOURCES += $$PWD/qcups.cpp
        HEADERS += $$PWD/qcups_p.h
} else {
        DEFINES += QT_NO_CUPS QT_NO_LPR
}
