# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(TARGET Qt6::Gui AND TARGET Qt6::Widgets)
    if(TARGET Qt6::uic)
        add_subdirectory(qgraphicsitemgroup_testsetup)
    endif()
    add_subdirectory(qlayout_testsetup)
    add_subdirectory(qconcatenateproxymodel_testsetup)
endif()
