HEADERS   = mainwindow.h \
            textedit.h
SOURCES   = main.cpp \
            mainwindow.cpp \
            textedit.cpp
RESOURCES = customcompleter.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/tools/customcompleter
INSTALLS += target

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
