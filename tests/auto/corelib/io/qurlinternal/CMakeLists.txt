# Generated from qurlinternal.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qurlinternal Test:
#####################################################################

qt_internal_add_test(tst_qurlinternal
    SOURCES
        tst_qurlinternal.cpp
        utf8data.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

#### Keys ignored in scope 1:.:.:qurlinternal.pro:<TRUE>:
# _REQUIREMENTS = "qtConfig(private_tests)"
