# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

list(APPEND test_data "data/image.png")
list(APPEND test_data "data/image@2x.png")

qt_internal_add_test(tst_qtextimagehandler
    SOURCES
    tst_qtextimagehandler.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA
        ${test_data}
)

qt_internal_add_resource(tst_qtextimagehandler "qtextimagehandler"
    PREFIX
        "/"
    FILES
        ${test_data}
)
