# Qt util module

HEADERS += \
        util/qsystemtrayicon.h \
        util/qcolormap.h \
        util/qsystemtrayicon_p.h

SOURCES += \
        util/qsystemtrayicon.cpp \
        util/qcolormap.cpp

qtConfig(completer) {
    HEADERS += \
        util/qcompleter.h \
        util/qcompleter_p.h

    SOURCES += util/qcompleter.cpp
}

qtConfig(scroller) {
    HEADERS += \
        util/qscroller.h \
        util/qscroller_p.h \
        util/qscrollerproperties.h \
        util/qscrollerproperties_p.h \
        util/qflickgesture_p.h

    SOURCES += \
        util/qscroller.cpp \
        util/qscrollerproperties.cpp \
        util/qflickgesture.cpp \
}

qtConfig(undocommand) {
    HEADERS += \
        util/qundostack.h \
        util/qundostack_p.h

    SOURCES += util/qundostack.cpp
}

qtConfig(undogroup) {
    HEADERS += util/qundogroup.h
    SOURCES += util/qundogroup.cpp
}

qtConfig(undoview) {
    HEADERS += util/qundoview.h
    SOURCES += util/qundoview.cpp
}

qtConfig(xcb) {
    SOURCES += util/qsystemtrayicon_x11.cpp
} else {
    SOURCES += util/qsystemtrayicon_qpa.cpp
}

mac {
    OBJECTIVE_SOURCES += util/qscroller_mac.mm
}
