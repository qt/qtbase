# Qt time / date / zone / calendar module

HEADERS += \
        time/qdatetime.h \
        time/qdatetime_p.h

SOURCES += time/qdatetime.cpp

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
