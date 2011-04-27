HEADERS += Test.h
SOURCES += main.cpp Test.cpp
QT += network
contains(QT_CONFIG, qt3support): QT += qt3support

CONFIG -= app_bundle
CONFIG += console
DESTDIR = ./
MOC_DIR = .moc/
TMP_DIR = .tmp/

symbian: TARGET.CAPABILITY = NetworkServices

