HEADERS       = mainwindow.h \
                xbelreader.h \
                xbelwriter.h
SOURCES       = main.cpp \
                mainwindow.cpp \
                xbelreader.cpp \
                xbelwriter.cpp
QT           += xml widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/xml/streambookmarks
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS streambookmarks.pro *.xbel
sources.path = $$[QT_INSTALL_EXAMPLES]/xml/streambookmarks
INSTALLS += target sources
