# Generated from findtestdata.pro.

#####################################################################
## findtestdata Binary:
#####################################################################

add_qt_executable(findtestdata
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        findtestdata.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

# Resources:
set(findtestdata_resource_files
    "qt.conf"
)

add_qt_resource(findtestdata "findtestdata"
    PREFIX
        "/qt/etc/"
    FILES
        ${findtestdata_resource_files}
)


## Scopes:
#####################################################################
