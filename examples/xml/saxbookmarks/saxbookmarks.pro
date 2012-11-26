HEADERS       = mainwindow.h \
                xbelgenerator.h \
                xbelhandler.h
SOURCES       = main.cpp \
                mainwindow.cpp \
                xbelgenerator.cpp \
                xbelhandler.cpp
QT           += xml widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/xml/saxbookmarks
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS saxbookmarks.pro *.xbel
sources.path = $$[QT_INSTALL_EXAMPLES]/xml/saxbookmarks
INSTALLS += target sources

wince*: {
     addFiles.files = frank.xbel jennifer.xbel
     addFiles.path = "\\My Documents"
     DEPLOYMENT += addFiles
}
