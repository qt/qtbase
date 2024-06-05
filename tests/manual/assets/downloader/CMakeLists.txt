# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_manual_downloader Binary:
#####################################################################

qt_internal_add_manual_test(tst_manual_downloader
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::ExamplesAssetDownloaderPrivate
        Qt::Widgets
)
