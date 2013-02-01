QT += widgets

HEADERS   = mainwindow.h \
            textedit.h
SOURCES   = main.cpp \
            mainwindow.cpp \
            textedit.cpp
RESOURCES = customcompleter.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tools/customcompleter
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
