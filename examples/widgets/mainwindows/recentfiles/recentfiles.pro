HEADERS       = mainwindow.h
SOURCES       = main.cpp \
                mainwindow.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/mainwindows/recentfiles
INSTALLS += target

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
