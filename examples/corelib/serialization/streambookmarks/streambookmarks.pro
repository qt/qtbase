HEADERS       = mainwindow.h \
                xbelreader.h \
                xbelwriter.h
SOURCES       = main.cpp \
                mainwindow.cpp \
                xbelreader.cpp \
                xbelwriter.cpp
QT           += widgets
requires(qtConfig(filedialog))

EXAMPLE_FILES = jennifer.xbel

# install
target.path = $$[QT_INSTALL_EXAMPLES]/corelib/serialization/streambookmarks
INSTALLS += target
