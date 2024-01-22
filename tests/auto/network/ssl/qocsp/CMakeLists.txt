# Generated from qocsp.pro.

#####################################################################
## tst_qocsp Test:
#####################################################################

qt_internal_add_test(tst_qocsp
    SOURCES
        tst_qocsp.cpp
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::NetworkPrivate
)

qt_internal_extend_target(tst_qocsp CONDITION QT_FEATURE_openssl_linked
    LIBRARIES
        WrapOpenSSL::WrapOpenSSL
)


## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:qocsp.pro:(CMAKE_BUILD_TYPE STREQUAL Debug):
# DESTDIR = "debug"

#### Keys ignored in scope 4:.:.:qocsp.pro:else:
# DESTDIR = "release"
