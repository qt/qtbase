TEMPLATE = app
TARGET = testlib_cppsnippet
QT = core testlib sql widgets

SOURCES += code/src_qtestlib_qtestcase.cpp \
           code/doc_src_qtestlib.cpp \
           code/doc_src_qtestevent.cpp \
           code/doc_src_qtqskip.cpp \
           code/doc_src_qttest.cpp \
           code/src_corelib_kernel_qtestsupport_core.cpp

load(qt_common)
