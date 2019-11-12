# Generated from network_remote_stresstest.pro.

#####################################################################
## tst_network_remote_stresstest Binary:
#####################################################################

add_qt_manual_test(tst_network_remote_stresstest
    SOURCES
        tst_network_remote_stresstest.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::Test
)

# Resources:
set(url-list_resource_files
    "url-list.txt"
)

add_qt_resource(tst_network_remote_stresstest "url-list"
    PREFIX
        "/"
    FILES
        ${url-list_resource_files}
)

