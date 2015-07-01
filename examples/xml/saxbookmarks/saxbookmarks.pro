HEADERS       = mainwindow.h \
                xbelgenerator.h \
                xbelhandler.h
SOURCES       = main.cpp \
                mainwindow.cpp \
                xbelgenerator.cpp \
                xbelhandler.cpp
QT           += xml widgets

EXAMPLE_FILES = frank.xbel jennifer.xbel

# install
target.path = $$[QT_INSTALL_EXAMPLES]/xml/saxbookmarks
INSTALLS += target

wince {
     addFiles.files = frank.xbel jennifer.xbel
     addFiles.path = "\\My Documents"
     INSTALLS += addFiles
}
