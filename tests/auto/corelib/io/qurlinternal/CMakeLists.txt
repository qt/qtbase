# Generated from qurlinternal.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qurlinternal Test:
#####################################################################

add_qt_test(tst_qurlinternal
    SOURCES
        ../../codecs/utf8/utf8data.cpp
        tst_qurlinternal.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

#### Keys ignored in scope 1:.:.:qurlinternal.pro:<TRUE>:
# _REQUIREMENTS = "qtConfig(private_tests)"
