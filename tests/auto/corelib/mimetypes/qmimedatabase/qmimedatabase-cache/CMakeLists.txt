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
set_source_files_properties("../invalid-magic1.xml"
    PROPERTIES QT_RESOURCE_ALIAS "invalid-magic1.xml"
)
set_source_files_properties("../invalid-magic2.xml"
    PROPERTIES QT_RESOURCE_ALIAS "invalid-magic2.xml"
)
set_source_files_properties("../invalid-magic3.xml"
    PROPERTIES QT_RESOURCE_ALIAS "invalid-magic3.xml"
)
set_source_files_properties("../magic-and-hierarchy.foo"
    PROPERTIES QT_RESOURCE_ALIAS "magic-and-hierarchy.foo"
)
set_source_files_properties("../magic-and-hierarchy.xml"
    PROPERTIES QT_RESOURCE_ALIAS "magic-and-hierarchy.xml"
)
set_source_files_properties("../magic-and-hierarchy2.foo"
    PROPERTIES QT_RESOURCE_ALIAS "magic-and-hierarchy2.foo"
)
set_source_files_properties("../qml-again.xml"
    PROPERTIES QT_RESOURCE_ALIAS "qml-again.xml"
)
set_source_files_properties("../test.qml"
    PROPERTIES QT_RESOURCE_ALIAS "test.qml"
)
set_source_files_properties("../text-x-objcsrc.xml"
    PROPERTIES QT_RESOURCE_ALIAS "text-x-objcsrc.xml"
)
set_source_files_properties("../yast2-metapackage-handler-mimetypes.xml"
    PROPERTIES QT_RESOURCE_ALIAS "yast2-metapackage-handler-mimetypes.xml"
)
set(testdata_resource_files
    "invalid-magic1.xml"
    "invalid-magic2.xml"
    "invalid-magic3.xml"
    "magic-and-hierarchy.foo"
    "magic-and-hierarchy.xml"
    "magic-and-hierarchy2.foo"
    "qml-again.xml"
    "test.qml"
    "text-x-objcsrc.xml"
    "yast2-metapackage-handler-mimetypes.xml"
)

qt_internal_add_resource(tst_qmimedatabase-cache "testdata"
    PREFIX
        "/qt-project.org/qmime"
    BASE
        ".."
    FILES
        ${testdata_resource_files}
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
