HEADERS       = mainwindow.h \
                xbeltree.h
SOURCES       = main.cpp \
                mainwindow.cpp \
                xbeltree.cpp
QT           += xml widgets
requires(qtConfig(filedialog))

EXAMPLE_FILES = frank.xbel jennifer.xbel

# install
target.path = $$[QT_INSTALL_EXAMPLES]/xml/dombookmarks
INSTALLS += target
