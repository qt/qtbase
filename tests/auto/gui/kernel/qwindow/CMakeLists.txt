# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qwindow Test:
#####################################################################

qt_internal_add_test(tst_qwindow
    SOURCES
        tst_qwindow.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

if(APPLE OR WIN32)
    qt_internal_add_test(tst_foreignwindow
        SOURCES
            tst_foreignwindow.cpp
        LIBRARIES
            Qt::CorePrivate
            Qt::Gui
            Qt::GuiPrivate
    )

    if(APPLE)
        set_source_files_properties(tst_foreignwindow.cpp PROPERTIES LANGUAGE OBJCXX)
        set_property(TARGET tst_foreignwindow PROPERTY PROPERTY MACOSX_BUNDLE TRUE)
    endif()
endif()

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qwindow CONDITION QT_FEATURE_dynamicgl AND WIN32
    LIBRARIES
        user32
)
