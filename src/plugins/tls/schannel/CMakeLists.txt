# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_plugin(QSchannelBackendPlugin
    OUTPUT_NAME qschannelbackend
    CLASS_NAME QSchannelBackend
    PLUGIN_TYPE tls
    DEFAULT_IF WIN32
    SOURCES
        ../shared/qtlskey_base_p.h
        ../shared/qtlskey_base.cpp
        ../shared/qtlskey_generic_p.h
        ../shared/qtlskey_generic.cpp
        ../shared/qx509_base_p.h
        ../shared/qx509_base.cpp
        ../shared/qx509_generic_p.h
        ../shared/qx509_generic.cpp
        ../shared/qsslsocket_qt.cpp
        ../shared/qwincrypt_p.h
        ../shared/qasn1element_p.h
        ../shared/qasn1element.cpp
        qtls_schannel.cpp qtls_schannel_p.h
        qtlsbackend_schannel_p.h
        qtlskey_schannel.cpp qtlskey_schannel_p.h
        qx509_schannel.cpp qx509_schannel_p.h
    LIBRARIES
        Qt::NetworkPrivate
        crypt32
        secur32
        bcrypt
        ncrypt
    DEFINES
        QT_NO_CAST_FROM_ASCII
)
