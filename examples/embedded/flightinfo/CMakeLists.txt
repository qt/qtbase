# Generated from flightinfo.pro.

#####################################################################
## flightinfo Binary:
#####################################################################

add_qt_executable(flightinfo
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/embedded/flightinfo"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/embedded/flightinfo"
    SOURCES
        flightinfo.cpp
        form.ui
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

# Resources:
add_qt_resource(flightinfo "flightinfo" PREFIX "/" FILES
    aircraft.png)


#### Keys ignored in scope 1:.:.:flightinfo.pro:<TRUE>:
# TEMPLATE = "app"
