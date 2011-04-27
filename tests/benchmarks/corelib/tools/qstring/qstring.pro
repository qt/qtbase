load(qttest_p4)
TARGET = tst_bench_qstring
QT -= gui
SOURCES += main.cpp data.cpp fromlatin1.cpp fromutf8.cpp

wince*:{
   DEFINES += SRCDIR=\\\"\\\"
} else:symbian* {
   addFiles.files = utf-8.txt
   addFiles.path = .
   DEPLOYMENT += addFiles
   TARGET.EPOCHEAPSIZE="0x100 0x1000000"
} else {
   DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

sse4:QMAKE_CXXFLAGS += -msse4
else:ssse3:QMAKE_FLAGS += -mssse3
else:sse2:QMAKE_CXXFLAGS += -msse2
neon:QMAKE_CXXFLAGS += -mfpu=neon
