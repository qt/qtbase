// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    visible: true
    implicitWidth: 1200
    implicitHeight: 500

    property AbstractItemModel model
    Text {
        text: "Data from " + (model ? model.objectName : "<empty>")
        x: 0
        y: parent.height - height
    }

    SplitView {
        anchors.fill: parent
        ListView {
            id: list
            SplitView.preferredWidth: 200
            model: root.model
            delegateModelAccess: DelegateModel.ReadWrite
            delegate: RowLayout {
                id: delegate
                width: ListView.view.width
                required property string display
                required property int row
                required property int column
                required property int index
                required property var modelData
                required property var model
                Text {
                    text: delegate.display
                    Layout.preferredWidth: 50
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        ToolTip.visible: containsMouse
                        ToolTip.text: delegate.modelData + " (" + delegate.model + ")"
                        onClicked: !isNaN(delegate.display) ? ++delegate.display
                                                            : delegate.display += "!"
                    }
                }
            }
        }

        TableView {
            id: table
            SplitView.preferredWidth: 200
            model: root.model
            alternatingRows: true
            rowSpacing: 5
            columnSpacing: 5
            clip: true
            selectionModel: ItemSelectionModel {}
            delegate: TableViewDelegate {
                implicitWidth: 50
                implicitHeight: 20
            }
        }

        TreeView {
            id: treeView
            SplitView.preferredWidth: 200
            clip: true
            selectionModel: ItemSelectionModel {}
            model: root.model

            delegate: TreeViewDelegate {}
        }
    }
}
