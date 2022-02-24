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

if(QT_FEATURE_process)
    add_dependencies(tst_qtextstream stdinProcess readAllStdinProcess readLineStdinProcess)
endif()
