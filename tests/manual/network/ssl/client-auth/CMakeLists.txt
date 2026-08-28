# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_manual_ssl_client_auth LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

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
