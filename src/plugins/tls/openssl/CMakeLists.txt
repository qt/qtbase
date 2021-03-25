qt_internal_add_plugin(QTlsBackendOpenSSL
    OUTPUT_NAME opensslbackend
    CLASS_NAME QTlsBackendOpenSSL
    TYPE tls
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
    PUBLIC_LIBRARIES
        Qt::NetworkPrivate
        Qt::CorePrivate
    DEFINES
        OPENSSL_API_COMPAT=0x10100000L
)

qt_internal_extend_target(QTlsBackendOpenSSL CONDITION QT_FEATURE_dtls
    SOURCES
        qdtls_openssl.cpp qdtls_openssl_p.h
        ../shared/qdtls_base.cpp ../shared/qdtls_base_p.h
)

qt_internal_extend_target(QTlsBackendOpenSSL CONDITION APPLE
    SOURCES
        ../shared/qsslsocket_mac_shared.cpp
    LIBRARIES
        ${FWCoreFoundation}
        ${FWSecurity}
)

qt_internal_extend_target(QTlsBackendOpenSSL CONDITION ANDROID AND NOT ANDROID_EMBEDDED
    SOURCES
        qsslsocket_openssl_android.cpp
)

qt_internal_extend_target(QTlsBackendOpenSSL CONDITION QT_FEATURE_openssl
                          AND QT_FEATURE_ssl AND WIN32
    SOURCES
        qwindowscarootfetcher.cpp qwindowscarootfetcher_p.h
        ../shared/qwincrypt_p.h
    LIBRARIES
        crypt32
)

