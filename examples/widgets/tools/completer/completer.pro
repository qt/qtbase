HEADERS   = fsmodel.h \
            mainwindow.h
SOURCES   = fsmodel.cpp \
            main.cpp \
            mainwindow.cpp
RESOURCES = completer.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/tools/completer
INSTALLS += target

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
