HEADERS     = movieplayer.h
SOURCES     = main.cpp \
              movieplayer.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/movie
sources.files = $$SOURCES $$HEADERS $$RESOURCES movie.pro animation.mng
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/movie
INSTALLS += target sources

symbian: CONFIG += qt_example

wince*: {
   addFiles.files += *.mng
   addFiles.path = .
   DEPLOYMENT += addFiles
   DEPLOYMENT_PLUGIN += qmng
}

