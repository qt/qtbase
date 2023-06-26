# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_example(completer)
qt_internal_add_example(customcompleter)
qt_internal_add_example(regularexpression)
qt_internal_add_example(settingseditor)
qt_internal_add_example(styleplugin)
qt_internal_add_example(treemodelcompleter)
qt_internal_add_example(undoframework)

if(QT_FEATURE_library)
    qt_internal_add_example(echoplugin)
endif()
