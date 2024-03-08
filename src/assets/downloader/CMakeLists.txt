# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_module(ExamplesAssetDownloaderPrivate
    CONFIG_MODULE_NAME examples_asset_downloader
    STATIC
    EXCEPTIONS
    INTERNAL_MODULE
    SOURCES
        assetdownloader.cpp assetdownloader.h
    LIBRARIES
        Qt6::Concurrent
        Qt6::CorePrivate
        Qt6::Network
    PUBLIC_LIBRARIES
        Qt6::Core
)

