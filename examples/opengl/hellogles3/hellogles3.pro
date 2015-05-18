HEADERS = $$PWD/glwidget.h \
          $$PWD/mainwindow.h \
          $$PWD/../hellogl2/logo.h

SOURCES = $$PWD/glwidget.cpp \
          $$PWD/main.cpp \
          $$PWD/mainwindow.cpp \
          $$PWD/../hellogl2/logo.cpp

QT += widgets

RESOURCES += hellogles3.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/opengl/hellogles3
INSTALLS += target
