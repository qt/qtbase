// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    anchors.fill: parent
    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        ToolBar {
            RowLayout {
                anchors.fill: parent
                ToolButton {
                    id: normalButton
                    checkable: true
                    icon.name: checked ? iconNameOn.text : iconNameOff.text
                }
                ToolButton {
                    id: checkedButton
                    checked: true
                    checkable: true
                    icon.name: checked ? iconNameOn.text : iconNameOff.text
                }
                ToolButton {
                    id: disabledButton
                    enabled: false
                    icon.name: checked ? iconNameOn.text : iconNameOff.text
                }
            }
        }

        RowLayout {
            Label {
                text: "Off:"
            }
            TextField {
                id: iconNameOff
                text: "mail-mark-read"
            }
            Label {
                text: "On:"
            }
            TextField {
                id: iconNameOn
                text: "mail-mark-unread"
            }
        }
    }
}
