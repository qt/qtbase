# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(QT_FEATURE_securetransport)
    add_subdirectory(securetransport)
endif()

if (QT_FEATURE_openssl OR QT_FEATURE_openssl_linked)
    add_subdirectory(openssl)
endif()

if (QT_FEATURE_schannel)
    add_subdirectory(schannel)
endif()

add_subdirectory(certonly)
