if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(android_content_uri LANGUAGES CXX)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_content_uris
    SOURCES
        tst_content_uris.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Widgets
)
