# Qt data formats core module

HEADERS += \
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
    serialization/qdatastream.cpp \
    serialization/qjson.cpp \
    serialization/qjsondocument.cpp \
    serialization/qjsonobject.cpp \
    serialization/qjsonarray.cpp \
    serialization/qjsonvalue.cpp \
    serialization/qjsonwriter.cpp \
    serialization/qjsonparser.cpp \
    serialization/qtextstream.cpp \
    serialization/qxmlstream.cpp \
    serialization/qxmlutils.cpp
