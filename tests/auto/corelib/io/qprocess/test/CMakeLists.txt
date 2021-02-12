# Generated from test.pro.

#####################################################################
## tst_qprocess Test:
#####################################################################

qt_internal_add_test(tst_qprocess
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        ../tst_qprocess.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Network
)

#### Keys ignored in scope 1:.:.:test.pro:<TRUE>:
# TEST_HELPER_INSTALLS = "../testProcessSpacesArgs/nospace" "../testProcessSpacesArgs/one space" "../testProcessSpacesArgs/two space s" "../test Space In Name/testSpaceInName"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:test.pro:WIN32:
# TESTDATA = "../testBatFiles/*"

#### Keys ignored in scope 5:.:..:../qprocess.pri:NOT TARGET Qt::Widgets:
# SUBPROGRAMS = "-testGuiProcess"
