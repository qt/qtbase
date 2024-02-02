// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

Rectangle {
    anchors.fill: parent
    Column {
        Row {
            ToolButton {
                id: normalButton
                icon.name: iconName.text
            }
            ToolButton {
                id: disabledButton
                enabled: false
                icon.name: iconName.text
            }
            ToolButton {
                id: checkedButton
                checked: true
                icon.name: iconName.text
            }
        }
        TextField {
            id: iconName
            text: "folder"
        }
    }
}
