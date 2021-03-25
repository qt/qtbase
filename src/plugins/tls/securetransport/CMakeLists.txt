qt_internal_add_plugin(QSecureTransportBackend
    OUTPUT_NAME securetransportbackend
    CLASS_NAME QSecureTransportBackend
    TYPE tls
    DEFAULT_IF APPLE
    SOURCES
        ../shared/qsslsocket_mac_shared.cpp
        ../shared/qtlskey_generic_p.h
        ../shared/qtlskey_generic.cpp
        ../shared/qx509_base_p.h
        ../shared/qx509_base.cpp
        ../shared/qx509_generic_p.h
        ../shared/qx509_generic.cpp
        ../shared/qtlskey_base_p.h
        ../shared/qtlskey_base.cpp
        ../shared/qsslsocket_qt.cpp
        ../shared/qasn1element_p.h
        ../shared/qasn1element.cpp
        qtlsbackend_st.cpp
        qtlsbackend_st_p.h
        qx509_st.cpp
        qtlskey_st.cpp
        qtlskey_st_p.h
        qx509_st_p.h
        qtls_st.cpp
        qtls_st_p.h
    PUBLIC_LIBRARIES
        Qt::NetworkPrivate
        Qt::CorePrivate
    LIBRARIES
        ${FWCoreFoundation}
        ${FWSecurity}
)
