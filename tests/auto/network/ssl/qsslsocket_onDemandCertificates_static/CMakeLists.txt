# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qsslsocket_onDemandCertificates_static Test:
#####################################################################

qt_internal_add_test(tst_qsslsocket_onDemandCertificates_static
    SOURCES
        tst_qsslsocket_onDemandCertificates_static.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    QT_TEST_SERVER_LIST "squid" "danted"
    BUNDLE_ANDROID_OPENSSL_LIBS
)
