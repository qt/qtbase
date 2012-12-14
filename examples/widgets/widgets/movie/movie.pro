QT += widgets

HEADERS     = movieplayer.h
SOURCES     = main.cpp \
              movieplayer.cpp

EXAMPLE_FILES = animation.gif

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/movie
INSTALLS += target


wince*: {
   addFiles.files += *.gif
   addFiles.path = .
   DEPLOYMENT += addFiles
}

simulator: warning(This example might not fully work on Simulator platform)
