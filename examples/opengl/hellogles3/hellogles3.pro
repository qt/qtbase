QT += opengl

HEADERS = $$PWD/glwindow.h \
          $$PWD/logo.h

SOURCES = $$PWD/glwindow.cpp \
          $$PWD/main.cpp \
          $$PWD/logo.cpp

RESOURCES += hellogles3.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/opengl/hellogles3
INSTALLS += target
