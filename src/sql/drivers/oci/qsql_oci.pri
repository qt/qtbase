HEADERS += $$PWD/qsql_oci.h
SOURCES += $$PWD/qsql_oci.cpp

unix {
    !contains(LIBS, .*clnts.*):LIBS += -lclntsh
} else {
    LIBS *= -loci
}
macx:QMAKE_LFLAGS += -Wl,-flat_namespace,-U,_environ
