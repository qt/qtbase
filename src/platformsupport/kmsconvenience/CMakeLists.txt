# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from kmsconvenience.pro.

qt_find_package(Libdrm) # special case

#####################################################################
## KmsSupportPrivate Module:
#####################################################################

qt_internal_add_module(KmsSupportPrivate
    CONFIG_MODULE_NAME kms_support
    STATIC
    INTERNAL_MODULE
    SOURCES
        qkmsdevice.cpp qkmsdevice_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    PUBLIC_LIBRARIES
        Libdrm::Libdrm
        Qt::CorePrivate
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:.:kmsconvenience.pro:<TRUE>:
# MODULE = "kms_support"
