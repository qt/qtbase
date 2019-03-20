# Generated from part3.pro.

#####################################################################
## part3 Binary:
#####################################################################

add_qt_executable(part3
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/addressbook/part3"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/addressbook/part3"
    SOURCES
        addressbook.cpp addressbook.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:part3.pro:<NONE>:
# QMAKE_PROJECT_NAME = "ab_part3"
