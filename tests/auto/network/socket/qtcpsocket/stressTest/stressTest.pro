HEADERS += Test.h
SOURCES += main.cpp Test.cpp
QT = core network testlib

CONFIG += cmdline
DESTDIR = ./
MOC_DIR = .moc/
TMP_DIR = .tmp/
