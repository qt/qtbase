# Generated from movie.pro.

#####################################################################
## movie Binary:
#####################################################################

add_qt_executable(movie
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/movie"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/movie"
    SOURCES
        main.cpp
        movieplayer.cpp movieplayer.h
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:movie.pro:<NONE>:
# EXAMPLE_FILES = "animation.gif"
