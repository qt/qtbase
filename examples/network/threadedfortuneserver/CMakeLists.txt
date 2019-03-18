# Generated from threadedfortuneserver.pro.

#####################################################################
## threadedfortuneserver Binary:
#####################################################################

add_qt_executable(threadedfortuneserver
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/threadedfortuneserver"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/threadedfortuneserver"
    SOURCES
        dialog.cpp dialog.h
        fortuneserver.cpp fortuneserver.h
        fortunethread.cpp fortunethread.h
        main.cpp
    LIBRARIES
        Qt::Network
        Qt::Widgets
)
