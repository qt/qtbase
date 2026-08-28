#####################################################################
## tst_wl_reconnect Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_wl_reconnect LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_wl_reconnect
    SOURCES
        wl-socket.c
        tst_reconnect.cpp
    LIBRARIES
        SharedClientTest
)
