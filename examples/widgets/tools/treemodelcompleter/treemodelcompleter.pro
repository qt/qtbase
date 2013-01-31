QT += widgets

HEADERS   = treemodelcompleter.h \
            mainwindow.h
SOURCES   = treemodelcompleter.cpp \
            main.cpp \
            mainwindow.cpp
RESOURCES = treemodelcompleter.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tools/treemodelcompleter
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
