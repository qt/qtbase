# Generated from stressTest.pro.

#####################################################################
## stressTest Binary:
#####################################################################

add_qt_executable(stressTest
    SOURCES
        Test.cpp Test.h
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::Test
)

#### Keys ignored in scope 1:.:.:stressTest.pro:<TRUE>:
# CONFIG = "cmdline"
# DESTDIR = "./"
# MOC_DIR = ".moc/"
# TMP_DIR = ".tmp/"
