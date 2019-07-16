# Generated from qmimedatabase-cache.pro.

#####################################################################
## tst_qmimedatabase-cache Test:
#####################################################################

add_qt_test(tst_qmimedatabase-cache
    SOURCES
        ../tst_qmimedatabase.h
        tst_qmimedatabase-cache.cpp
    PUBLIC_LIBRARIES
        Qt::Concurrent
)

# Resources:
set_source_files_properties("../qml-again.xml"
    PROPERTIES alias "qml-again.xml"
)
set_source_files_properties("../test.qml"
    PROPERTIES alias "test.qml"
)
set_source_files_properties("../text-x-objcsrc.xml"
    PROPERTIES alias "text-x-objcsrc.xml"
)
set_source_files_properties("../yast2-metapackage-handler-mimetypes.xml"
    PROPERTIES alias "yast2-metapackage-handler-mimetypes.xml"
)
add_qt_resource(tst_qmimedatabase-cache "testdata"
    PREFIX
        "/qt-project.org/qmime"
    BASE
        ".."
    FILES
        invalid-magic1.xml
        invalid-magic2.xml
        invalid-magic3.xml
        magic-and-hierarchy.foo
        magic-and-hierarchy.xml
        magic-and-hierarchy2.foo
        qml-again.xml
        test.qml
        text-x-objcsrc.xml
        yast2-metapackage-handler-mimetypes.xml
)

# special case begin
set(corelib_source_dir ../../../../../../src/corelib)
include(${corelib_source_dir}/mimetypes/mimetypes_resources.cmake)
corelib_add_mimetypes_resources(tst_qmimedatabase-cache)
# special case end

#### Keys ignored in scope 1:.:.:qmimedatabase-cache.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

extend_target(tst_qmimedatabase-cache CONDITION CMAKE_COMPILER_IS_GNUCXX

    COMPILE_OPTIONS
        -W
        -Wall
        -Wextra
        -Wno-long-long
        -Wnon-virtual-dtor
        -Wshadow
)

extend_target(tst_qmimedatabase-cache CONDITION UNIX AND NOT APPLE_OSX AND NOT QNX
    DEFINES
        USE_XDG_DATA_DIRS
)
