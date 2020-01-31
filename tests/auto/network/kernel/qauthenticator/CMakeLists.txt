# Generated from qauthenticator.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qauthenticator Test:
#####################################################################

add_qt_test(tst_qauthenticator
    SOURCES
        tst_qauthenticator.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/\\\"
    LIBRARIES
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 1:.:.:qauthenticator.pro:<TRUE>:
# _REQUIREMENTS = "qtConfig(private_tests)"
