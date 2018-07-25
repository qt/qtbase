CONFIG += testcase console

QT = core-private network testlib
TARGET = tst_qobject
SOURCES = tst_qobject.cpp

# Force C++17 if available (needed due to P0012R1)
contains(QT_CONFIG, c++1z): CONFIG += c++1z

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
