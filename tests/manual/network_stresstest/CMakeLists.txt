# Generated from network_stresstest.pro.

#####################################################################
## tst_network_stresstest Binary:
#####################################################################

qt_internal_add_manual_test(tst_network_stresstest
    SOURCES
        minihttpserver.cpp minihttpserver.h
        tst_network_stresstest.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
        Qt::Test
)

# Resources:
set(wwwfiles_resource_files
    "qtest"
)

qt_internal_add_resource(tst_network_stresstest "wwwfiles"
    PREFIX
        "/"
    FILES
        ${wwwfiles_resource_files}
)


#### Keys ignored in scope 1:.:.:network_stresstest.pro:<TRUE>:
# QMAKE_RESOURCE_FLAGS = "-no-compress"
