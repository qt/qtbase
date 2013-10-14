QT += widgets

HEADERS       = mainwindow.h
SOURCES       = main.cpp \
                mainwindow.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/mainwindows/recentfiles
INSTALLS += target
