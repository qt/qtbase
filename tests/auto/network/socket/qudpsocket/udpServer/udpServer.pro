SOURCES += main.cpp
QT = core network
CONFIG -= app_bundle
CONFIG += console

symbian:TARGET.CAPABILITY="ALL -TCB"

