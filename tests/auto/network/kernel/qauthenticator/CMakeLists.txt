# Generated from qauthenticator.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qauthenticator Test:
#####################################################################

qt_internal_add_test(tst_qauthenticator
    SOURCES
        tst_qauthenticator.cpp
    PUBLIC_LIBRARIES
        Qt::NetworkPrivate
)

#### Keys ignored in scope 1:.:.:qauthenticator.pro:<TRUE>:
# _REQUIREMENTS = "qtConfig(private_tests)"
