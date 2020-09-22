# Generated from qdecompresshelper.pro.

#####################################################################
## tst_qdecompresshelper Test:
#####################################################################

qt_internal_add_test(tst_qdecompresshelper
    SOURCES
        gzip.rcc.cpp
        inflate.rcc.cpp
        tst_qdecompresshelper.cpp
        zstandard.rcc.cpp
    DEFINES
        SRC_DIR=${CMAKE_CURRENT_SOURCE_DIR} # special case
    PUBLIC_LIBRARIES
        Qt::NetworkPrivate
)

#### Keys ignored in scope 1:.:.:qdecompresshelper.pro:<TRUE>:
# TEMPLATE = "app"
