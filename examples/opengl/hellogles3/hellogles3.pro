HEADERS = $$PWD/glwindow.h \
          $$PWD/../hellogl2/logo.h

SOURCES = $$PWD/glwindow.cpp \
          $$PWD/main.cpp \
          $$PWD/../hellogl2/logo.cpp

RESOURCES += hellogles3.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/opengl/hellogles3
INSTALLS += target
