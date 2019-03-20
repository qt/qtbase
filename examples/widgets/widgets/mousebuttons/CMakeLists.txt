# Generated from mousebuttons.pro.

#####################################################################
## mousebuttons Binary:
#####################################################################

add_qt_executable(mousebuttons
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/mousebuttons"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/mousebuttons"
    SOURCES
        buttontester.cpp buttontester.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:mousebuttons.pro:<NONE>:
# TEMPLATE = "app"
