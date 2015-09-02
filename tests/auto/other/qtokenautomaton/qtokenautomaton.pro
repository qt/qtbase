CONFIG += testcase
TARGET = tst_qtokenautomaton
SOURCES += tst_qtokenautomaton.cpp                      \
           tokenizers/basic/basic.cpp                   \
           tokenizers/basicNamespace/basicNamespace.cpp \
           tokenizers/boilerplate/boilerplate.cpp       \
           tokenizers/noNamespace/noNamespace.cpp       \
           tokenizers/noToString/noToString.cpp         \
           tokenizers/withNamespace/withNamespace.cpp

HEADERS += tokenizers/basic/basic.h                     \
           tokenizers/basicNamespace/basicNamespace.h   \
           tokenizers/boilerplate/boilerplate.h         \
           tokenizers/noNamespace/noNamespace.h         \
           tokenizers/noToString/noToString.h           \
           tokenizers/withNamespace/withNamespace.h

QT = core testlib
