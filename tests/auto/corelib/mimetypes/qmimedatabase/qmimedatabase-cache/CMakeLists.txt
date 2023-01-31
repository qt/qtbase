# Generated from qmimedatabase-cache.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qmimedatabase-cache Test:
#####################################################################

qt_internal_add_test(tst_qmimedatabase-cache
    SOURCES
        ../tst_qmimedatabase.h
        tst_qmimedatabase-cache.cpp
    PUBLIC_LIBRARIES
        Qt::Concurrent
)

# Resources:
# special case begin
# the freedesktop resources are handled manually below via mimetypes_resources.cmake
#set(mimetypes_resource_files
    #"mime/packages/freedesktop.org.xml"
#)
# special case end
set(testdata_resource_files
    "../invalid-magic1.xml"
    "../invalid-magic2.xml"
    "../invalid-magic3.xml"
    "../magic-and-hierarchy.foo"
    "../magic-and-hierarchy.xml"
    "../magic-and-hierarchy2.foo"
    "../qml-again.xml"
    "../test.qml"
    "../text-x-objcsrc.xml"
    "../yast2-metapackage-handler-mimetypes.xml"
)

qt_internal_add_resource(tst_qmimedatabase-cache "testdata"
    PREFIX
        "/qt-project.org/qmime"
    BASE
        ".."
    FILES
        ${testdata_resource_files}
)

qt_internal_add_resource(tst_qmimedatabase-cache "testfiles"
    PREFIX
        "/files"
    FILES
        "../test.txt"
        "../test.qml"
)

# special case begin
set(corelib_source_dir ../../../../../../src/corelib)
include(${corelib_source_dir}/mimetypes/mimetypes_resources.cmake)
corelib_add_mimetypes_resources(tst_qmimedatabase-cache)
# special case end

#### Keys ignored in scope 1:.:.:qmimedatabase-cache.pro:<TRUE>:
# _REQUIREMENTS = "qtConfig(private_tests)"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qmimedatabase-cache CONDITION GCC
    COMPILE_OPTIONS
        -W
        -Wall
        -Wextra
        -Wno-long-long
        -Wnon-virtual-dtor
        -Wshadow
)

qt_internal_extend_target(tst_qmimedatabase-cache CONDITION UNIX AND NOT APPLE AND NOT QNX
    DEFINES
        USE_XDG_DATA_DIRS
)
