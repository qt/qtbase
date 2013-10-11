QT += widgets

HEADERS   = fsmodel.h \
            mainwindow.h
SOURCES   = fsmodel.cpp \
            main.cpp \
            mainwindow.cpp
RESOURCES = completer.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tools/completer
INSTALLS += target
