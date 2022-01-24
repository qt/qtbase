QT += widgets opengl openglwidgets

SOURCES += main.cpp \
           glwidget.cpp \
           mainwindow.cpp \
           renderer.cpp

HEADERS += glwidget.h \
           mainwindow.h \
           renderer.h

target.path = $$[QT_INSTALL_EXAMPLES]/opengl/threadedqopenglwidget
INSTALLS += target
