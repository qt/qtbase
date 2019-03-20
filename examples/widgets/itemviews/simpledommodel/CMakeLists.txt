# Generated from simpledommodel.pro.

#####################################################################
## simpledommodel Binary:
#####################################################################

add_qt_executable(simpledommodel
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/simpledommodel"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/simpledommodel"
    SOURCES
        domitem.cpp domitem.h
        dommodel.cpp dommodel.h
        main.cpp
        mainwindow.cpp mainwindow.h
    LIBRARIES
        Qt::Xml
        Qt::Widgets
)
