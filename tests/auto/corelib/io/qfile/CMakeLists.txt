# Generated from qfile.pro.

#####################################################################
## tst_qfile Test:
#####################################################################

# Collect test data
# list(APPEND test_data "BLACKLIST") # special case remove
list(APPEND test_data "dosfile.txt")
list(APPEND test_data "noendofline.txt")
list(APPEND test_data "testfile.txt")
list(APPEND test_data "testlog.txt")
list(APPEND test_data "two.dots.file")
list(APPEND test_data "tst_qfile.cpp")
# list(APPEND test_data "Makefile") # special case remove
list(APPEND test_data "forCopying.txt")
list(APPEND test_data "forRenaming.txt")
list(APPEND test_data "resources/file1.ext1")

add_qt_test(tst_qfile
    SOURCES
        ../../../../shared/emulationdetector.h
        tst_qfile.cpp
    INCLUDE_DIRECTORIES
        ../../../../shared
    PUBLIC_LIBRARIES
        Qt::CorePrivate
    TESTDATA ${test_data}
)

# Resources:
set(qfile_resource_files
    "resources/"
)

add_qt_resource(tst_qfile "qfile"
    PREFIX
        "/tst_qfileinfo/"
    FILES
        ${qfile_resource_files}
)
set(rename-fallback_resource_files
    "rename-fallback.qrc"
)

add_qt_resource(tst_qfile "rename-fallback"
    PREFIX
        "/"
    FILES
        ${rename-fallback_resource_files}
)
set(copy-fallback_resource_files
    "copy-fallback.qrc"
)

add_qt_resource(tst_qfile "copy-fallback"
    PREFIX
        "/"
    FILES
        ${copy-fallback_resource_files}
)


## Scopes:
#####################################################################

extend_target(tst_qfile CONDITION TARGET Qt::Network
    PUBLIC_LIBRARIES
        Qt::Network
)

extend_target(tst_qfile CONDITION NOT TARGET Qt::Network
    DEFINES
        QT_NO_NETWORK
)

extend_target(tst_qfile CONDITION CONFIG___contains___builtin_testdata
    DEFINES
        BUILTIN_TESTDATA
)

extend_target(tst_qfile CONDITION WIN32
    PUBLIC_LIBRARIES
        ole32
        uuid
)
add_subdirectory(stdinprocess)
