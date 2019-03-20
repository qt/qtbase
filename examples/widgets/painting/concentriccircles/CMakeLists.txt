# Generated from concentriccircles.pro.

#####################################################################
## concentriccircles Binary:
#####################################################################

add_qt_executable(concentriccircles
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/concentriccircles"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/concentriccircles"
    SOURCES
        circlewidget.cpp circlewidget.h
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)
