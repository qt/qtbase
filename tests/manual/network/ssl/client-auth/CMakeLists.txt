# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(tst_manual_ssl_client_auth
    SOURCES
        tst_manual_ssl_client_auth.cpp
    LIBRARIES
        Qt::Network
)

qt_internal_add_resource(tst_manual_ssl_client_auth "tst_manual_ssl_client_auth"
    PREFIX
        "/"
    FILES
        "certs/127.0.0.1.pem"
        "certs/127.0.0.1-key.pem"
        "certs/127.0.0.1-client.pem"
        "certs/127.0.0.1-client-key.pem"
        "certs/accepted-client.pem"
        "certs/accepted-client-key.pem"
        "certs/rootCA.pem"
    BASE
        "certs"
)
