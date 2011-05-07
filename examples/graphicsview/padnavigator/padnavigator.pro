SOURCES += main.cpp \
    roundrectitem.cpp \
    flippablepad.cpp \
    padnavigator.cpp \
    splashitem.cpp

HEADERS += \
    roundrectitem.h \
    flippablepad.h \
    padnavigator.h \
    splashitem.h

RESOURCES += \
    padnavigator.qrc

FORMS += \
    form.ui

contains(QT_CONFIG, opengl):QT += opengl widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/graphicsview/padnavigator
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS padnavigator.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/graphicsview/padnavigator
INSTALLS += target sources

CONFIG += console

symbian {
    TARGET.UID3 = 0xA000A644
    CONFIG += qt_example
}
