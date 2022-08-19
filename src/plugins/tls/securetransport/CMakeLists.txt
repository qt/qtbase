# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_plugin(QSecureTransportBackendPlugin
    OUTPUT_NAME qsecuretransportbackend
    CLASS_NAME QSecureTransportBackend
    PLUGIN_TYPE tls
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
    LIBRARIES
        Qt::NetworkPrivate
        Qt::CorePrivate
        ${FWCoreFoundation}
        ${FWSecurity}
)
