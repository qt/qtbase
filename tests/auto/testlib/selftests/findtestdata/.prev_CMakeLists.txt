# Generated from findtestdata.pro.

#####################################################################
## findtestdata Binary:
#####################################################################

qt_add_executable(findtestdata
    SOURCES
        findtestdata.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

# Resources:
set(findtestdata_resource_files
    "qt.conf"
)

qt_add_resource(findtestdata "findtestdata"
    PREFIX
        "/qt/etc/"
    FILES
        ${findtestdata_resource_files}
)


## Scopes:
#####################################################################
