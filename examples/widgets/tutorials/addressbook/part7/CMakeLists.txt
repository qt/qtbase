# Generated from part7.pro.

#####################################################################
## part7 Binary:
#####################################################################

add_qt_executable(part7
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/addressbook/part7"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/addressbook/part7"
    SOURCES
        addressbook.cpp addressbook.h
        finddialog.cpp finddialog.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:part7.pro:<NONE>:
# QMAKE_PROJECT_NAME = "ab_part7"
