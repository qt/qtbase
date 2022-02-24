# Generated from test.pro.

#####################################################################
## tst_qtextstream Test:
#####################################################################

# Collect test data
list(APPEND test_data "../rfc3261.txt")
list(APPEND test_data "../task113817.txt")
list(APPEND test_data "../qtextstream.qrc")
list(APPEND test_data "../qtextstream_integrity.qrc")
list(APPEND test_data "../tst_qtextstream.cpp")
list(APPEND test_data "../resources")
list(APPEND test_data "../BLACKLIST")

qt_internal_add_test(tst_qtextstream
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        ../tst_qtextstream.cpp
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::TestPrivate
    TESTDATA ${test_data}
    QT_TEST_SERVER_LIST "cyrus"
)

# Resources:
set(qtextstream_resource_files
    "../resources/big_endian/"
    "../resources/little_endian/"
)

qt_internal_add_resource(tst_qtextstream "qtextstream"
    PREFIX
        "/tst_textstream/"
    BASE
        ".."
    FILES
        ${qtextstream_resource_files}
)

if(INTEGRITY)
set(qtextstream_resource_files_integrity
    "../rfc3261.txt"
    "../task113817.txt"
    "../tst_qtextstream.cpp"
)

qt_internal_add_resource(tst_qtextstream "qtextstream_integrity"
    PREFIX
        "/"
    FILES
        ${qtextstream_resource_files_integrity}
)
endif()

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qtextstream CONDITION builtin_testdata OR INTEGRITY
    DEFINES
        BUILTIN_TESTDATA
)
