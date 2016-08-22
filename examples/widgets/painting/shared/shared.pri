INCLUDEPATH += $$PWD

qtHaveModule(opengl)|qtConfig(opengles2)  {
    DEFINES += QT_OPENGL_SUPPORT
    QT += opengl widgets
}

SOURCES += \
    $$PWD/arthurstyle.cpp\
    $$PWD/arthurwidgets.cpp \
    $$PWD/hoverpoints.cpp

HEADERS += \
    $$PWD/arthurstyle.h \
    $$PWD/arthurwidgets.h \
    $$PWD/hoverpoints.h

RESOURCES += $$PWD/shared.qrc

