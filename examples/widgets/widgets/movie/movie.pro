HEADERS     = movieplayer.h
SOURCES     = main.cpp \
              movieplayer.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/widgets/movie
sources.files = $$SOURCES $$HEADERS $$RESOURCES movie.pro animation.gif
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/widgets/movie
INSTALLS += target sources


wince*: {
   addFiles.files += *.gif
   addFiles.path = .
   DEPLOYMENT += addFiles
}

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
