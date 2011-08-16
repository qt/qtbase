# Qt util module

HEADERS += \
        util/qsystemtrayicon.h \
        util/qcolormap.h \
        util/qcompleter.h \
        util/qcompleter_p.h \
        util/qsystemtrayicon_p.h \
        util/qscroller.h \
        util/qscroller_p.h \
        util/qscrollerproperties.h \
        util/qscrollerproperties_p.h \
        util/qflickgesture_p.h \
        util/qundogroup.h \
        util/qundostack.h \
        util/qundostack_p.h \
        util/qundoview.h

SOURCES += \
        util/qsystemtrayicon.cpp \
        util/qcolormap_qpa.cpp \
        util/qcompleter.cpp \
        util/qscroller.cpp \
        util/qscrollerproperties.cpp \
        util/qflickgesture.cpp \
        util/qundogroup.cpp \
        util/qundostack.cpp \
        util/qundoview.cpp


wince* {
		SOURCES += \
				util/qsystemtrayicon_wince.cpp
} else:win32:!qpa {
		SOURCES += \
				util/qsystemtrayicon_win.cpp
}

unix:x11 {
		SOURCES += \
				util/qsystemtrayicon_x11.cpp
}

qpa {
		SOURCES += \
                                util/qsystemtrayicon_qpa.cpp
}

!qpa:!x11:mac {
		OBJECTIVE_SOURCES += util/qsystemtrayicon_mac.mm
}

symbian {
    LIBS += -letext -lplatformenv
    contains(S60_VERSION, 3.1)|contains(S60_VERSION, 3.2)|contains(S60_VERSION, 5.0) {
        LIBS += -lsendas2 -lapmime
        contains(QT_CONFIG, s60) {
            contains(CONFIG, is_using_gnupoc) {
                LIBS += -lcommonui
            } else {
                LIBS += -lCommonUI
            }
        }
    } else {
        DEFINES += USE_SCHEMEHANDLER
    }
}

macx {
    OBJECTIVE_SOURCES += util/qscroller_mac.mm
}
