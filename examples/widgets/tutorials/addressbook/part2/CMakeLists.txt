# Generated from part2.pro.

#####################################################################
## part2 Binary:
#####################################################################

add_qt_executable(part2
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/addressbook/part2"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/addressbook/part2"
    SOURCES
        addressbook.cpp addressbook.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:part2.pro:<NONE>:
# QMAKE_PROJECT_NAME = "ab_part2"
