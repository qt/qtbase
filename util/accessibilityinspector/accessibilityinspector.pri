QT += declarative
INCLUDEPATH += $$PWD

# DEFINES += ACCESSIBILITYINSPECTOR_NO_UITOOLS
# CONFIG += uitools

mac {
    # for text-to-speach
    LIBS += -framework AppKit
}

HEADERS += \
    $$PWD/screenreader.h \
    $$PWD/optionswidget.h \
    $$PWD/accessibilityscenemanager.h \
    $$PWD/accessibilityinspector.h
SOURCES += \
    $$PWD/optionswidget.cpp \
    $$PWD/accessibilityscenemanager.cpp \
    $$PWD/screenreader.cpp \
    $$PWD/accessibilityinspector.cpp

OBJECTIVE_SOURCES += $$PWD/screenreader_mac.mm


