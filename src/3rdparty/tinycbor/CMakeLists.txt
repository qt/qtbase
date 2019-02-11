add_library(tinycbor INTERFACE)
target_include_directories(tinycbor
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

qt_install_3rdparty_library(tinycbor)
