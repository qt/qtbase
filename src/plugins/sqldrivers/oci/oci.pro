TARGET = qsqloci

HEADERS += $$PWD/qsql_oci_p.h
SOURCES += $$PWD/qsql_oci.cpp $$PWD/main.cpp

unix {
    !contains(LIBS, .*clnts.*):LIBS += -lclntsh
} else {
    LIBS *= -loci
}
darwin:QMAKE_LFLAGS += -Wl,-flat_namespace,-U,_environ

OTHER_FILES += oci.json

PLUGIN_CLASS_NAME = QOCIDriverPlugin
include(../qsqldriverbase.pri)
