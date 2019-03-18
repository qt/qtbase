# Generated from wordcount.pro.

#####################################################################
## wordcount Binary:
#####################################################################

add_qt_executable(wordcount
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/qtconcurrent/wordcount"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/qtconcurrent/wordcount"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Concurrent
        Qt::Widgets
)

#### Keys ignored in scope 1:.:wordcount.pro:<NONE>:
# CONFIG = "-app_bundle"
