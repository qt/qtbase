# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

if(QT_FEATURE_wizard)
    qt_internal_add_example(trivialwizard)
    if(Qt6::PrintSupport)
        qt_internal_add_example(licensewizard)
    endif()
endif()
qt_internal_add_example(standarddialogs)
qt_internal_add_example(tabdialog)
