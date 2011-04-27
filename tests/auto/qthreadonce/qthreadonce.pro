load(qttest_p4)
SOURCES += tst_qthreadonce.cpp
QT = core

# Don't use gcc's threadsafe statics
# Note: some versions of gcc generate invalid code with this option...
# Some versions of gcc don't even have it, so disable it
#*-g++*:QMAKE_CXXFLAGS += -fno-threadsafe-statics

# Temporary:
SOURCES += qthreadonce.cpp

CONFIG += parallel_test
