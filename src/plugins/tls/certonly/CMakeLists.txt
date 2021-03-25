qt_internal_add_plugin(QTlsBackendCertOnly
    OUTPUT_NAME certonlybackend
    CLASS_NAME QTlsBackendCertOnly
    TYPE tls
    SOURCES
        ../shared/qx509_base_p.h
        ../shared/qx509_base.cpp
        ../shared/qx509_generic_p.h
        ../shared/qx509_generic.cpp
        ../shared/qasn1element_p.h
        ../shared/qasn1element.cpp
        qtlsbackend_cert.cpp
        qtlsbackend_cert_p.h
    PUBLIC_LIBRARIES
        Qt::NetworkPrivate
)
