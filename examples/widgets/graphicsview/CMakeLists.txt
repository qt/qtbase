# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

qt_internal_add_example(chip)
qt_internal_add_example(elasticnodes)
qt_internal_add_example(collidingmice)
qt_internal_add_example(basicgraphicslayouts)
qt_internal_add_example(diagramscene)
qt_internal_add_example(simpleanchorlayout)
if(QT_FEATURE_cursor AND QT_FEATURE_draganddrop)
    qt_internal_add_example(dragdroprobot)
endif()
