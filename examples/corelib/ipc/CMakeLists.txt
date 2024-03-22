# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

if(NOT TARGET Qt6::Widgets)
    return()
endif()
if(QT_FEATURE_sharedmemory AND QT_FEATURE_systemsemaphore)
    qt_internal_add_example(sharedmemory)
endif()
if(QT_FEATURE_localserver AND TARGET Qt6::Network)
    qt_internal_add_example(localfortuneserver)
    qt_internal_add_example(localfortuneclient)
endif()
