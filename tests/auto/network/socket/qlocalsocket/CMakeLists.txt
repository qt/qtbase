# Generated from qlocalsocket.pro.

# add_subdirectory(test) # special case remove
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
