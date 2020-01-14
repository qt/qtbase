# Generated from tst.pro.

#####################################################################
## tst_qpluginloader Test:
#####################################################################

# Collect test data
list(APPEND test_data "../elftest")
list(APPEND test_data "../machtest")

add_qt_test(tst_qpluginloader
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        ../fakeplugin.cpp
        ../theplugin/plugininterface.h
        ../tst_qpluginloader.cpp
    PUBLIC_LIBRARIES
        staticplugin
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

extend_target(tst_qpluginloader CONDITION QT_FEATURE_private_tests
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
)

extend_target(tst_qpluginloader CONDITION CMAKE_BUILD_TYPE STREQUAL Debug AND WIN32 AND debug_and_release
    PUBLIC_LIBRARIES
        # Remove: L../staticplugin/debug
)

extend_target(tst_qpluginloader CONDITION WIN32 AND debug_and_release AND NOT CMAKE_BUILD_TYPE STREQUAL Debug
    PUBLIC_LIBRARIES
        # Remove: L../staticplugin/release
)

extend_target(tst_qpluginloader CONDITION UNIX OR NOT debug_and_release
    PUBLIC_LIBRARIES
        # Remove: L../staticplugin
)
