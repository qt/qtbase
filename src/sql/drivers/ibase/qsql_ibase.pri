HEADERS += $$PWD/qsql_ibase.h
SOURCES += $$PWD/qsql_ibase.cpp

unix {
    !contains(LIBS, .*gds.*):!contains(LIBS, .*libfb.*):LIBS += -lgds
} else {
    !contains(LIBS, .*gds.*):!contains(LIBS, .*fbclient.*) {
        win32-borland:LIBS += gds32.lib
        else:LIBS += -lgds32_ms
    }
}
