HEADERS       = mainwindow.h \
                xbelreader.h \
                xbelwriter.h
SOURCES       = main.cpp \
                mainwindow.cpp \
                xbelreader.cpp \
                xbelwriter.cpp
QT           += xml

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/xml/streambookmarks
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS streambookmarks.pro *.xbel
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/xml/streambookmarks
INSTALLS += target sources

symbian: {
    CONFIG += qt_example
    addFiles.sources = frank.xbel jennifer.xbel
    addFiles.path = /data/qt/streambookmarks
    DEPLOYMENT += addFiles
}
maemo5: CONFIG += qt_example
