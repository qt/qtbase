HEADERS       = mainwindow.h \
                xbelgenerator.h \
                xbelhandler.h
SOURCES       = main.cpp \
                mainwindow.cpp \
                xbelgenerator.cpp \
                xbelhandler.cpp
QT           += xml

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/xml/saxbookmarks
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS saxbookmarks.pro *.xbel
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/xml/saxbookmarks
INSTALLS += target sources

wince*: {
     addFiles.files = frank.xbel jennifer.xbel
     addFiles.path = "\\My Documents"
     DEPLOYMENT += addFiles
}

symbian: {
     TARGET.UID3 = 0xA000C60A
     include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
     addFiles.files = frank.xbel jennifer.xbel
     addFiles.path = /data/qt/saxbookmarks
     DEPLOYMENT += addFiles
}
