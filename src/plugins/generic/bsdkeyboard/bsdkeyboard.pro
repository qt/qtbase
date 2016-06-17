TARGET = qbsdkeyboardplugin

PLUGIN_TYPE = generic
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QBsdKeyboardPlugin
load(qt_plugin)

QT += core gui-private

HEADERS = qbsdkeyboard.h
SOURCES = main.cpp \
         qbsdkeyboard.cpp

OTHER_FILES += \
    qbsdkeyboard.json

