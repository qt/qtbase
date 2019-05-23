# Generated from securesocketclient.pro.

#####################################################################
## securesocketclient Binary:
#####################################################################

add_qt_executable(securesocketclient
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/securesocketclient"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/securesocketclient"
    SOURCES
        certificateinfo.cpp certificateinfo.h certificateinfo.ui
        main.cpp
        sslclient.cpp sslclient.h sslclient.ui
        sslerrors.ui
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

# Resources:
add_qt_resource(securesocketclient "securesocketclient" FILES
    encrypted.png)

