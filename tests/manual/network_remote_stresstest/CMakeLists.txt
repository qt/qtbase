# Generated from network_remote_stresstest.pro.

#####################################################################
## tst_network_remote_stresstest Binary:
#####################################################################

qt_internal_add_manual_test(tst_network_remote_stresstest
    SOURCES
        tst_network_remote_stresstest.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Network
        Qt::NetworkPrivate
        Qt::Test
)

# Resources:
set(url-list_resource_files
    "url-list.txt"
)

qt_internal_add_resource(tst_network_remote_stresstest "url-list"
    PREFIX
        "/"
    FILES
        ${url-list_resource_files}
)

