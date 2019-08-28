# Qt time / date / zone / calendar module

HEADERS += \
        time/qcalendar.h \
        time/qcalendarbackend_p.h \
        time/qcalendarmath_p.h \
        time/qdatetime.h \
        time/qdatetime_p.h \
        time/qgregoriancalendar_p.h \
        time/qjuliancalendar_p.h \
        time/qmilankoviccalendar_p.h \
        time/qromancalendar_p.h \
        time/qromancalendar_data_p.h

SOURCES += \
        time/qdatetime.cpp \
        time/qcalendar.cpp \
        time/qgregoriancalendar.cpp \
        time/qjuliancalendar.cpp \
        time/qmilankoviccalendar.cpp \
        time/qromancalendar.cpp

qtConfig(hijricalendar) {
    SOURCES += \
        time/qhijricalendar.cpp
    HEADERS += \
        time/qhijricalendar_p.h \
        time/qhijricalendar_data_p.h
}

qtConfig(islamiccivilcalendar) {
    SOURCES += \
        time/qislamiccivilcalendar.cpp
    HEADERS += \
        time/qislamiccivilcalendar_p.h
}

qtConfig(jalalicalendar) {
    SOURCES += \
        time/qjalalicalendar.cpp
    HEADERS += \
        time/qjalalicalendar_p.h \
        time/qjalalicalendar_data_p.h
}

qtConfig(timezone) {
    HEADERS += \
        time/qtimezone.h \
        time/qtimezoneprivate_p.h \
        time/qtimezoneprivate_data_p.h
    SOURCES += \
        time/qtimezone.cpp \
        time/qtimezoneprivate.cpp
    !nacl:darwin: {
        SOURCES += time/qtimezoneprivate_mac.mm
    } else: android:!android-embedded: {
        SOURCES += time/qtimezoneprivate_android.cpp
    } else: unix: {
        SOURCES += time/qtimezoneprivate_tz.cpp
        qtConfig(icu): SOURCES += time/qtimezoneprivate_icu.cpp
    } else: qtConfig(icu): {
        SOURCES += time/qtimezoneprivate_icu.cpp
    } else: win32: {
        SOURCES += time/qtimezoneprivate_win.cpp
    }
}

qtConfig(datetimeparser) {
    HEADERS += time/qdatetimeparser_p.h
    SOURCES += time/qdatetimeparser.cpp
}
