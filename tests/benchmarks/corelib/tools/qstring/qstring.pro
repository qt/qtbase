TARGET = tst_bench_qstring
QT -= gui
QT += core-private testlib
SOURCES += main.cpp data.cpp fromlatin1.cpp fromutf8.cpp

wince*:{
   DEFINES += SRCDIR=\\\"\\\"
} else {
   DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

sse4:QMAKE_CXXFLAGS += -msse4
else:ssse3:QMAKE_FLAGS += -mssse3
else:sse2:QMAKE_CXXFLAGS += -msse2
neon:QMAKE_CXXFLAGS += -mfpu=neon
