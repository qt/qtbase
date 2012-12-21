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

QT += widgets
qtHaveModule(opengl): QT += opengl

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/graphicsview/padnavigator
INSTALLS += target

CONFIG += console
