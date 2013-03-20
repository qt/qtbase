SOURCES     = windowcontainer.cpp

QT += widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/windowcontainer
INSTALLS += target

include(../../gui/openglwindow/openglwindow.pri)
