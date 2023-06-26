QT += widgets
requires(qtConfig(filedialog))

HEADERS     = movieplayer.h
SOURCES     = main.cpp \
              movieplayer.cpp

EXAMPLE_FILES = animation.gif

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/movie
INSTALLS += target
