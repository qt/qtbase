QT += concurrent widgets
CONFIG += console
CONFIG -= app_bundle

SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtconcurrent/wordcount
INSTALLS += target
