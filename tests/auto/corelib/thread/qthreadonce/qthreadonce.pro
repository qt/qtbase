CONFIG += testcase parallel_test
TARGET = tst_qthreadonce
QT = core testlib
SOURCES = tst_qthreadonce.cpp

# Don't use gcc's threadsafe statics
# Note: some versions of gcc generate invalid code with this option...
# Some versions of gcc don't even have it, so disable it
#*-g++*:QMAKE_CXXFLAGS += -fno-threadsafe-statics

# Temporary:
SOURCES += qthreadonce.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
