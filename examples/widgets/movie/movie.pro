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

maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
