# Qt data formats core module

HEADERS += \
    serialization/qcborarray.h \
    serialization/qcborcommon.h \
    serialization/qcborcommon_p.h \
    serialization/qcbormap.h \
    serialization/qcborstream.h \
    serialization/qcborvalue.h \
    serialization/qcborvalue_p.h \
    serialization/qdatastream.h \
    serialization/qdatastream_p.h \
    serialization/qjson_p.h \
    serialization/qjsondocument.h \
    serialization/qjsonobject.h \
    serialization/qjsonvalue.h \
    serialization/qjsonarray.h \
    serialization/qjsonwriter_p.h \
    serialization/qjsonparser_p.h \
    serialization/qtextstream.h \
    serialization/qtextstream_p.h \
    serialization/qxmlstream.h \
    serialization/qxmlstream_p.h \
    serialization/qxmlutils_p.h

SOURCES += \
    serialization/qcborcommon.cpp \
    serialization/qcbordiagnostic.cpp \
    serialization/qcborvalue.cpp \
    serialization/qdatastream.cpp \
    serialization/qjsoncbor.cpp \
    serialization/qjsondocument.cpp \
    serialization/qjsonobject.cpp \
    serialization/qjsonarray.cpp \
    serialization/qjsonvalue.cpp \
    serialization/qjsonwriter.cpp \
    serialization/qjsonparser.cpp \
    serialization/qtextstream.cpp \
    serialization/qxmlstream.cpp \
    serialization/qxmlutils.cpp

qtConfig(cborstreamreader): {
    SOURCES += \
        serialization/qcborstreamreader.cpp

    HEADERS += \
        serialization/qcborstreamreader.h
}

qtConfig(cborstreamwriter): {
    SOURCES += \
        serialization/qcborstreamwriter.cpp

    HEADERS += \
        serialization/qcborstreamwriter.h
}

qtConfig(binaryjson): {
    HEADERS += \
        serialization/qbinaryjson_p.h \
        serialization/qbinaryjsonarray_p.h \
        serialization/qbinaryjsonobject_p.h \
        serialization/qbinaryjsonvalue_p.h

    SOURCES += \
        serialization/qbinaryjson.cpp \
        serialization/qbinaryjsonarray.cpp \
        serialization/qbinaryjsonobject.cpp \
        serialization/qbinaryjsonvalue.cpp \
}

false: SOURCES += \
    serialization/qcborarray.cpp \
    serialization/qcbormap.cpp

INCLUDEPATH += ../3rdparty/tinycbor/src
