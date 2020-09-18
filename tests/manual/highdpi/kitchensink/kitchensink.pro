TEMPLATE = app
TARGET = kitchensink
INCLUDEPATH += .
QT += widgets gui-private
CONFIG += cmdline
CONFIG += c++11
# Input
SOURCES += \
        dragwidget.cpp \
        main.cpp

HEADERS += \
        dragwidget.h

RESOURCES += \
    kitchensink.qrc

DEFINES += HAVE_SCREEN_BASE_DPI
