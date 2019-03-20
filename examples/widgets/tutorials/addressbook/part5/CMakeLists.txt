# Generated from part5.pro.

#####################################################################
## part5 Binary:
#####################################################################

add_qt_executable(part5
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/addressbook/part5"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/addressbook/part5"
    SOURCES
        addressbook.cpp addressbook.h
        finddialog.cpp finddialog.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:part5.pro:<NONE>:
# QMAKE_PROJECT_NAME = "ab_part5"
