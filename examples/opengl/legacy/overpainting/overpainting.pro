QT += opengl widgets

VPATH += ../shared
INCLUDEPATH += ../shared

HEADERS = bubble.h \
    glwidget.h \
    qtlogo.h

SOURCES = bubble.cpp \
    glwidget.cpp \
    main.cpp \
    qtlogo.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/legacy/overpainting
INSTALLS += target

qtConfig(opengles.|angle): error("This example requires Qt to be configured with -opengl desktop")
