# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_plugin(QTlsBackendOpenSSLPlugin
    OUTPUT_NAME qopensslbackend
    CLASS_NAME QTlsBackendOpenSSL
    PLUGIN_TYPE tls
    SOURCES
        ../shared/qx509_base.cpp ../shared/qx509_base_p.h
        ../shared/qtlskey_base.cpp ../shared/qtlskey_base_p.h
        ../shared/qasn1element.cpp ../shared/qasn1element_p.h
        qtlsbackend_openssl.cpp qtlsbackend_openssl_p.h
        qx509_openssl.cpp qx509_openssl_p.h
        qtlskey_openssl.cpp qtlskey_openssl_p.h
        qtls_openssl.cpp qtls_openssl_p.h
        qssldiffiehellmanparameters_openssl.cpp
        qsslcontext_openssl.cpp qsslcontext_openssl_p.h
        qsslsocket_openssl_symbols.cpp qsslsocket_openssl_symbols_p.h
        qopenssl_p.h
    LIBRARIES
        Qt::NetworkPrivate
        Qt::CorePrivate
    DEFINES
        OPENSSL_API_COMPAT=0x10100000L
)

if (WIN32) # Windows header issues
    set_target_properties(QTlsBackendOpenSSLPlugin PROPERTIES UNITY_BUILD OFF)
endif()

qt_internal_extend_target(QTlsBackendOpenSSLPlugin CONDITION QT_FEATURE_dtls
    SOURCES
        qdtls_openssl.cpp qdtls_openssl_p.h
        ../shared/qdtls_base.cpp ../shared/qdtls_base_p.h
)

qt_internal_extend_target(QTlsBackendOpenSSLPlugin CONDITION APPLE
    SOURCES
        ../shared/qsslsocket_mac_shared.cpp
    LIBRARIES
        ${FWCoreFoundation}
        ${FWSecurity}
)

qt_internal_extend_target(QTlsBackendOpenSSLPlugin CONDITION ANDROID
    SOURCES
        qsslsocket_openssl_android.cpp
)

qt_internal_extend_target(QTlsBackendOpenSSLPlugin CONDITION WIN32
    SOURCES
        qwindowscarootfetcher.cpp qwindowscarootfetcher_p.h
        ../shared/qwincrypt_p.h
    LIBRARIES
        crypt32
)

if(QT_FEATURE_openssl_linked)
    target_link_libraries(QTlsBackendOpenSSLPlugin PRIVATE WrapOpenSSL::WrapOpenSSL)
else()
    qt_internal_add_target_include_dirs(QTlsBackendOpenSSLPlugin
                                        WrapOpenSSLHeaders::WrapOpenSSLHeaders)
endif()
