CONFIG += testcase
TARGET = tst_qhostinfo

SOURCES  += tst_qhostinfo.cpp

requires(qtConfig(private_tests))
QT = core-private network-private testlib

win32:LIBS += -lws2_32

# needed for getaddrinfo with official MinGW
mingw:DEFINES += _WIN32_WINNT=0x0501

winrt: WINRT_MANIFEST.capabilities += internetClientServer
