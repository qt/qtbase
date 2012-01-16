TEMPLATE = app
QT = core network
CONFIG += console

# Input
SOURCES += qget.cpp
HEADERS += qget.h

symbian: TARGET.CAPABILITY += ReadUserData NetworkServices

