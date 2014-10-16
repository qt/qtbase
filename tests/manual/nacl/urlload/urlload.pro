TEMPLATE = app
TARGET = urlload
DEPENDPATH += .
               .
# Input
SOURCES += main.cpp

LIBS +=  -lppapi -lnacl_io
QT = core network gui