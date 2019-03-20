# Generated from orderform.pro.

#####################################################################
## orderform Binary:
#####################################################################

add_qt_executable(orderform
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/richtext/orderform"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/richtext/orderform"
    SOURCES
        detailsdialog.cpp detailsdialog.h
        main.cpp
        mainwindow.cpp mainwindow.h
    LIBRARIES
        Qt::Widgets
)

## Scopes:
#####################################################################

extend_target(orderform CONDITION TARGET Qt::PrintSupport
    LIBRARIES
        Qt::PrintSupport
)
