/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 1.0

Rectangle {
    id: box
    width: 350; height: 250

    Rectangle {
        id: redSquare
        width: 80; height: 80
        anchors.top: parent.top; anchors.left: parent.left; anchors.margins: 10
        color: "red"

        Text { text: "Click"; font.pixelSize: 16; anchors.centerIn: parent }

        MouseArea {
            anchors.fill: parent 
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton

            onEntered: info.text = 'Entered'
            onExited: info.text = 'Exited (pressed=' + pressed + ')'

            onPressed: {
                info.text = 'Pressed (button=' + (mouse.button == Qt.RightButton ? 'right' : 'left') 
                    + ' shift=' + (mouse.modifiers & Qt.ShiftModifier ? 'true' : 'false') + ')'
                var posInBox = redSquare.mapToItem(box, mouse.x, mouse.y)
                posInfo.text = + mouse.x + ',' + mouse.y + ' in square'
                        + ' (' + posInBox.x + ',' + posInBox.y + ' in window)'
            }

            onReleased: {
                info.text = 'Released (isClick=' + mouse.isClick + ' wasHeld=' + mouse.wasHeld + ')'
                posInfo.text = ''
            }

            onPressAndHold: info.text = 'Press and hold'
            onClicked: info.text = 'Clicked (wasHeld=' + mouse.wasHeld + ')'
            onDoubleClicked: info.text = 'Double clicked'
        }
    }

    Rectangle {
        id: blueSquare
        width: 80; height: 80
        x: box.width - width - 10; y: 10    // making this item draggable, so don't use anchors
        color: "blue"

        Text { text: "Drag"; font.pixelSize: 16; color: "white"; anchors.centerIn: parent }

        MouseArea {
            anchors.fill: parent
            drag.target: blueSquare
            drag.axis: Drag.XandYAxis
            drag.minimumX: 0
            drag.maximumX: box.width - parent.width
            drag.minimumY: 0
            drag.maximumY: box.height - parent.width
        }
    }

    Text {
        id: info
        anchors.bottom: posInfo.top; anchors.horizontalCenter: parent.horizontalCenter; anchors.margins: 30

        onTextChanged: console.log(text)
    }

    Text {
        id: posInfo
        anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; anchors.margins: 30
    }
}
