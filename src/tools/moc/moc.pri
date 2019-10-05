
INCLUDEPATH += $$PWD \
    $$PWD/../../3rdparty/tinycbor/src

HEADERS =  $$PWD/moc.h \
           $$PWD/preprocessor.h \
           $$PWD/parser.h \
           $$PWD/symbols.h \
           $$PWD/token.h \
           $$PWD/utils.h \
           $$PWD/generator.h \
           $$PWD/outputrevision.h \
           $$PWD/cbordevice.h \
           $$PWD/collectjson.h

SOURCES =  $$PWD/moc.cpp \
           $$PWD/preprocessor.cpp \
           $$PWD/generator.cpp \
           $$PWD/parser.cpp \
           $$PWD/token.cpp \
           $$PWD/collectjson.cpp
