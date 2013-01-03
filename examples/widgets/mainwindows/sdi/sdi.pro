QT += widgets

HEADERS       = mainwindow.h
SOURCES       = main.cpp \
                mainwindow.cpp
RESOURCES     = sdi.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/mainwindows/sdi
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
