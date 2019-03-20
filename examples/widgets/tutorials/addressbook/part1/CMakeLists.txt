# Generated from part1.pro.

#####################################################################
## part1 Binary:
#####################################################################

add_qt_executable(part1
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/addressbook/part1"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/addressbook/part1"
    SOURCES
        addressbook.cpp addressbook.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:part1.pro:<NONE>:
# QMAKE_PROJECT_NAME = "ab_part1"
