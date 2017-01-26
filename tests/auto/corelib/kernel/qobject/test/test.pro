CONFIG += testcase console
TARGET = ../tst_qobject
QT = core-private network testlib
SOURCES = ../tst_qobject.cpp

# Force C++17 if available (needed due to P0012R1)
contains(QT_CONFIG, c++1z): CONFIG += c++1z

!winrt: TEST_HELPER_INSTALLS = ../signalbug/signalbug
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
