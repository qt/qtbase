QT += widgets

HEADERS       = mainwindow.h \
                mdichild.h
SOURCES       = main.cpp \
                mainwindow.cpp \
                mdichild.cpp
RESOURCES     = mdi.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/mainwindows/mdi
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
