# Generated from test.pro.

#####################################################################
## tst_qtextstream Test:
#####################################################################

# Collect test data
list(APPEND test_data "../rfc3261.txt")
list(APPEND test_data "../task113817.txt")
list(APPEND test_data "../qtextstream.qrc")
list(APPEND test_data "../tst_qtextstream.cpp")
list(APPEND test_data "../resources")
list(APPEND test_data "../BLACKLIST")

add_qt_test(tst_qtextstream
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        ../../../../../shared/emulationdetector.h
        ../tst_qtextstream.cpp
    INCLUDE_DIRECTORIES
        ../../../../../shared
    PUBLIC_LIBRARIES
        Qt::Network
    TESTDATA ${test_data}
)

# Resources:
set_source_files_properties("../resources/big_endian/"
    PROPERTIES QT_RESOURCE_ALIAS "resources/big_endian/"
)
set_source_files_properties("../resources/little_endian/"
    PROPERTIES QT_RESOURCE_ALIAS "resources/little_endian/"
)
set(qtextstream_resource_files
    "resources/big_endian/"
    "resources/little_endian/"
)

add_qt_resource(tst_qtextstream "qtextstream"
    PREFIX
        "/tst_textstream/"
    BASE
        ".."
    FILES
        ${qtextstream_resource_files}
)


## Scopes:
#####################################################################

extend_target(tst_qtextstream CONDITION builtin_testdata
    DEFINES
        BUILTIN_TESTDATA
)
