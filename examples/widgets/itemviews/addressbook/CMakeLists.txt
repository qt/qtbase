# Generated from addressbook.pro.

#####################################################################
## addressbook Binary:
#####################################################################

add_qt_executable(addressbook
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/addressbook"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/addressbook"
    SOURCES
        adddialog.cpp adddialog.h
        addresswidget.cpp addresswidget.h
        main.cpp
        mainwindow.cpp mainwindow.h
        newaddresstab.cpp newaddresstab.h
        tablemodel.cpp tablemodel.h
    LIBRARIES
        Qt::Widgets
)
