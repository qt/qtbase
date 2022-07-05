# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0



#### Inputs



#### Libraries



#### Tests



#### Features

qt_feature("dom" PUBLIC
    SECTION "File I/O"
    LABEL "DOM"
    PURPOSE "Supports the Document Object Model."
)
qt_feature_definition("dom" "QT_NO_DOM" NEGATE VALUE "1")
