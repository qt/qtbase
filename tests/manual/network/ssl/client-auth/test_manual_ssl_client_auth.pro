TEMPLATE = app
TARGET = tst_manual_ssl_client_auth
CONFIG += cmdline
QT = network

SOURCES += tst_manual_ssl_client_auth.cpp

cert_bundle.files = \
    certs/127.0.0.1.pem \
    certs/127.0.0.1-key.pem \
    certs/127.0.0.1-client.pem \
    certs/127.0.0.1-client-key.pem \
    certs/accepted-client.pem \
    certs/accepted-client-key.pem \
    certs/rootCA.pem
cert_bundle.base = certs

RESOURCES += cert_bundle
