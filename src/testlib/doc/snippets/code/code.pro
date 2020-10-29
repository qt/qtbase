TEMPLATE = lib
TARGET = code_snippets
QT += core sql widgets

#! [qmake_use]
QT += testlib
#! [qmake_use]

SOURCES = \
    doc_src_qtestevent.cpp \
    doc_src_qtestlib.cpp \
    doc_src_qtqskip.cpp \
    doc_src_qttest.cpp \
    src_corelib_kernel_qtestsupport_core.cpp

