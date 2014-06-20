QT += widgets

SOURCES += main.cpp \
           glwidget.cpp \
           mainwindow.cpp \
           bubble.cpp

HEADERS += glwidget.h \
           mainwindow.h \
           bubble.h

RESOURCES += texture.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/opengl/qopenglwidget
INSTALLS += target
