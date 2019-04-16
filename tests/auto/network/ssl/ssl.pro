TEMPLATE=subdirs
QT_FOR_CONFIG += network-private

SUBDIRS=\
   qpassworddigestor \
   qsslcertificate \
   qsslcipher \
   qsslellipticcurve \
   qsslerror \
   qsslkey \

qtConfig(ssl) {
    qtConfig(private_tests) {
        SUBDIRS += \
            qsslsocket \
            qsslsocket_onDemandCertificates_member \
            qsslsocket_onDemandCertificates_static

        qtConfig(dtls) {
            SUBDIRS += \
                qdtlscookie \
                qdtls
        }

        qtConfig(ocsp): SUBDIRS += qocsp
    }
}

winrt: SUBDIRS -= \
   qsslsocket_onDemandCertificates_member \
   qsslsocket_onDemandCertificates_static \

qtConfig(ssl) {
    qtConfig(private_tests) {
        SUBDIRS += qasn1element \
                   qssldiffiehellmanparameters
    }
}
