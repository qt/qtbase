HEADERS       = mainwindow.h \
                xbeltree.h
SOURCES       = main.cpp \
                mainwindow.cpp \
                xbeltree.cpp
QT           += xml

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/xml/dombookmarks
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS dombookmarks.pro *.xbel
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/xml/dombookmarks
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)

wince*: {
     addFiles.files = frank.xbel jennifer.xbel
     addFiles.path = "\\My Documents"
     DEPLOYMENT += addFiles
}
