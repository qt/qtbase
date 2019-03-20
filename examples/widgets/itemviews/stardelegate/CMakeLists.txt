# Generated from stardelegate.pro.

#####################################################################
## stardelegate Binary:
#####################################################################

add_qt_executable(stardelegate
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/stardelegate"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/stardelegate"
    SOURCES
        main.cpp
        stardelegate.cpp stardelegate.h
        stareditor.cpp stareditor.h
        starrating.cpp starrating.h
    LIBRARIES
        Qt::Widgets
)
