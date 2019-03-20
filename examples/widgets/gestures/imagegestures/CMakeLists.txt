# Generated from imagegestures.pro.

#####################################################################
## imagegestures Binary:
#####################################################################

add_qt_executable(imagegestures
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/gestures/imagegestures"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/gestures/imagegestures"
    SOURCES
        imagewidget.cpp imagewidget.h
        main.cpp
        mainwidget.cpp mainwidget.h
    LIBRARIES
        Qt::Widgets
)
