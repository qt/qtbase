# Generated from qfile.pro.

#####################################################################
## tst_qfile Test:
#####################################################################

add_qt_test(tst_qfile
    SOURCES
        ../../../../shared/emulationdetector.h
        tst_qfile.cpp
    INCLUDE_DIRECTORIES
        ../../../../shared
    LIBRARIES
        Qt::CorePrivate
)

# Resources:
add_qt_resource(tst_qfile "qfile" PREFIX "/tst_qfileinfo/" FILES
    resources/)
add_qt_resource(tst_qfile "rename-fallback" FILES
    rename-fallback.qrc)
add_qt_resource(tst_qfile "copy-fallback" FILES
    copy-fallback.qrc)


#### Keys ignored in scope 3:.:.:test.pro:<TRUE>:
# CONFIG = "testcase"
# TESTDATA = "BLACKLIST" "dosfile.txt" "noendofline.txt" "testfile.txt" "testlog.txt" "two.dots.file" "tst_qfile.cpp" "Makefile" "forCopying.txt" "forRenaming.txt" "resources/file1.ext1"

## Scopes:
#####################################################################

extend_target(tst_qfile CONDITION TARGET Qt::Network
    PUBLIC_LIBRARIES
        Qt::Network
)

extend_target(tst_qfile CONDITION NOT (TARGET Qt::Network)
    DEFINES
        QT_NO_NETWORK
)

extend_target(tst_qfile CONDITION CONFIG___contains___builtin_testdata
    DEFINES
        BUILTIN_TESTDATA
)

extend_target(tst_qfile CONDITION WIN32 AND NOT WINRT
    PUBLIC_LIBRARIES
        ole32
        uuid
)

if(NOT WINRT)
    add_subdirectory(stdinprocess)
endif()
