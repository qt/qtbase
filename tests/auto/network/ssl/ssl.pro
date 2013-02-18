TEMPLATE=subdirs
SUBDIRS=\
   qsslcertificate \
   qsslcipher \
   qsslerror \
   qsslkey \

contains(QT_CONFIG, openssl) | contains(QT_CONFIG, openssl-linked):
    contains(QT_CONFIG, private_tests) {
        SUBDIRS += \
            qsslsocket \
            qsslsocket_onDemandCertificates_member \
            qsslsocket_onDemandCertificates_static \
    }
