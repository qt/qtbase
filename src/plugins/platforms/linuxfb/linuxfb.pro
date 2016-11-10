TARGET = qlinuxfb

DEFINES += QT_NO_FOREACH

QT += \
    core-private gui-private \
    service_support-private eventdispatcher_support-private \
    fontdatabase_support-private fb_support-private

qtHaveModule(input_support-private): \
    QT += input_support-private

SOURCES = main.cpp \
          qlinuxfbintegration.cpp \
          qlinuxfbscreen.cpp

HEADERS = qlinuxfbintegration.h \
          qlinuxfbscreen.h

qtHaveModule(kms_support-private) {
    QT += kms_support-private
    SOURCES += qlinuxfbdrmscreen.cpp
    HEADERS += qlinuxfbdrmscreen.h
}

OTHER_FILES += linuxfb.json

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QLinuxFbIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)
