# Generated from stressTest.pro.

#####################################################################
## stressTest Binary:
#####################################################################

qt_internal_add_executable(stressTest
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        Test.cpp Test.h
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::Test
)

#### Keys ignored in scope 1:.:.:stressTest.pro:<TRUE>:
# MOC_DIR = ".moc/"
# TMP_DIR = ".tmp/"
