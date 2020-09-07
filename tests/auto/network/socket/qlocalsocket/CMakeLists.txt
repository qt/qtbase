# special case begin
add_subdirectory(socketprocess)

qt_internal_add_test(tst_qlocalsocket
    SOURCES
        tst_qlocalsocket.cpp
    DEFINES
        QLOCALSERVER_DEBUG
        QLOCALSOCKET_DEBUG
    PUBLIC_LIBRARIES
        Qt::Network
)
# special case end
