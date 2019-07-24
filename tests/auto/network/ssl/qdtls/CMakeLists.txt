# Generated from qdtls.pro.

#####################################################################
## tst_qdtls Test:
#####################################################################

# Collect test data
list(APPEND test_data "certs")

add_qt_test(tst_qdtls
    GUI
    SOURCES
        tst_qdtls.cpp
    LIBRARIES
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qdtls.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:qdtls.pro:(CMAKE_BUILD_TYPE STREQUAL Debug):
# DESTDIR = "debug"

#### Keys ignored in scope 4:.:.:qdtls.pro:else:
# DESTDIR = "release"
