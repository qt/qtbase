QT += widgets

SOURCES += main.cpp \
           glwidget.cpp \
           mainwindow.cpp

HEADERS += glwidget.h \
           mainwindow.h

target.path = $$[QT_INSTALL_EXAMPLES]/opengl/threadedqopenglwidget
INSTALLS += target
