# Generated from basicdrawing.pro.

#####################################################################
## basicdrawing Binary:
#####################################################################

add_qt_executable(basicdrawing
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/basicdrawing"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/basicdrawing"
    SOURCES
        main.cpp
        renderarea.cpp renderarea.h
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(basicdrawing "basicdrawing" FILES
    images/brick.png
    images/qt-logo.png)

