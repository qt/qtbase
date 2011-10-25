CONFIG += testcase
TARGET = tst_qplugin

SOURCES = tst_qplugin.cpp
QT = core testlib

wince*: {
   plugins.files = plugins/*
   plugins.path = plugins
   DEPLOYMENT += plugins
}
