CONFIG -= moc
QT += core-private   # for harfbuzz
INCLUDEPATH += . /usr/include/freetype2

SOURCES += main.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
