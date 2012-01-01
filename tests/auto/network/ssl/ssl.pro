TEMPLATE=subdirs
SUBDIRS=\
   qsslcertificate \
   qsslcipher \
   qsslerror \
   qsslkey \
   qsslsocket \
   qsslsocket_onDemandCertificates_member \
   qsslsocket_onDemandCertificates_static \

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
   qsslsocket \
   qsslsocket_onDemandCertificates_member \
   qsslsocket_onDemandCertificates_static \
