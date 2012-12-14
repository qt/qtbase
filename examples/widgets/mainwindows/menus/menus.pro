QT += widgets

HEADERS       = mainwindow.h
SOURCES       = mainwindow.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/mainwindows/menus
INSTALLS += target
