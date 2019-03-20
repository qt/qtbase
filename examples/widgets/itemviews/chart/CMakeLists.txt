# Generated from chart.pro.

#####################################################################
## chart Binary:
#####################################################################

add_qt_executable(chart
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/chart"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/chart"
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
        pieview.cpp pieview.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(chart "chart" PREFIX "/Charts" FILES
    qtdata.cht)


## Scopes:
#####################################################################

extend_target(chart CONDITION UNIX AND NOT APPLE_OSX AND NOT HAIKU AND NOT INTEGRITY AND NOT VXWORKS
    LIBRARIES
        m
)
