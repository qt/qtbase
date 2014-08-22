TEMPLATE=subdirs
SUBDIRS=\
   qsslcertificate \
   qsslcipher \
   qsslellipticcurve \
   qsslerror \
   qsslkey \

contains(QT_CONFIG, ssl) | contains(QT_CONFIG, openssl) | contains(QT_CONFIG, openssl-linked) {
    contains(QT_CONFIG, private_tests) {
        SUBDIRS += \
            qsslsocket \
            qsslsocket_onDemandCertificates_member \
            qsslsocket_onDemandCertificates_static \
    }
}

winrt: SUBDIRS -= \
   qsslsocket_onDemandCertificates_member \
   qsslsocket_onDemandCertificates_static \

contains(QT_CONFIG, ssl) | contains(QT_CONFIG, openssl) | contains(QT_CONFIG, openssl-linked) {
    contains(QT_CONFIG, private_tests) {
        SUBDIRS += qasn1element
    }
}
