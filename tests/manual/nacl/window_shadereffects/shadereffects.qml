/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtQml.Models 2.1
import "content"

Rectangle {
    id: root
    width: 320
    height: 480
    property color col: "lightsteelblue"
    gradient: Gradient {
        GradientStop { position: 0.0; color: Qt.tint(root.col, "#20FFFFFF") }
        GradientStop { position: 0.1; color: Qt.tint(root.col, "#20AAAAAA") }
        GradientStop { position: 0.9; color: Qt.tint(root.col, "#20666666") }
        GradientStop { position: 1.0; color: Qt.tint(root.col, "#20000000") }
    }

    //! [source]
    ShaderEffectSource {
        id: theSource
        sourceItem: theItem
    }
    //! [source]

    function saturate(x) {
        return Math.min(Math.max(x, 0), 1)
    }

    function sliderToColor(x) {
        return Qt.rgba(saturate(Math.max(2 - 6 * x, 6 * x - 4)),
                        saturate(Math.min(6 * x, 4 - 6 * x)),
                        saturate(Math.min(6 * x - 2, 6 - 6 * x)))
    }

    Grid {
        anchors.centerIn: parent
        columns: 2

        Item {
            id: theItem
            width: 160
            height: 160
            ListView {
                anchors.centerIn: parent
                width: 160
                height: 140
                clip: true
                snapMode: ListView.SnapOneItem
                model: ObjectModel {
                    Text {
                        width: 160
                        height: 140
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 120
                        font.family: "Times"
                        color: "blue"
                        text: "Qt"
                    }
                    Image {
                        width: 160
                        height: 140
                        source: "content/qt-logo.png"
                    }
                    Image {
                        width: 160
                        height: 140
                        source: "content/face-smile.png"
                    }
                }
            }
        }
        ShaderEffect {
            width: 160
            height: 160
            property variant source: theSource
            property real amplitude: 0.04 * wobbleSlider.value
            property real frequency: 20
            property real time: 0
            NumberAnimation on time { loops: Animation.Infinite; from: 0; to: Math.PI * 2; duration: 600 }
            //! [fragment]
            fragmentShader:
                "uniform lowp float qt_Opacity;" +
                "uniform highp float amplitude;" +
                "uniform highp float frequency;" +
                "uniform highp float time;" +
                "uniform sampler2D source;" +
                "varying highp vec2 qt_TexCoord0;" +
                "void main() {" +
                "    highp vec2 p = sin(time + frequency * qt_TexCoord0);" +
                "    gl_FragColor = texture2D(source, qt_TexCoord0 + amplitude * vec2(p.y, -p.x)) * qt_Opacity;" +
                "}"
            //! [fragment]
            Slider {
                id: wobbleSlider
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 40
            }
        }
        ShaderEffect {
            width: 160
            height: 160
            property variant source: theSource
            property variant shadow: ShaderEffectSource {
                sourceItem: ShaderEffect {
                    width: theItem.width
                    height: theItem.height
                    property variant delta: Qt.size(0.0, 1.0 / height)
                    property variant source: ShaderEffectSource {
                        sourceItem: ShaderEffect {
                            width: theItem.width
                            height: theItem.height
                            property variant delta: Qt.size(1.0 / width, 0.0)
                            property variant source: theSource
                            fragmentShader: "
                                uniform lowp float qt_Opacity;
                                uniform sampler2D source;
                                uniform highp vec2 delta;
                                varying highp vec2 qt_TexCoord0;
                                void main() {
                                    gl_FragColor =(0.0538 * texture2D(source, qt_TexCoord0 - 3.182 * delta)
                                                 + 0.3229 * texture2D(source, qt_TexCoord0 - 1.364 * delta)
                                                 + 0.2466 * texture2D(source, qt_TexCoord0)
                                                 + 0.3229 * texture2D(source, qt_TexCoord0 + 1.364 * delta)
                                                 + 0.0538 * texture2D(source, qt_TexCoord0 + 3.182 * delta)) * qt_Opacity;
                                }"
                        }
                    }
                    fragmentShader: "
                        uniform lowp float qt_Opacity;
                        uniform sampler2D source;
                        uniform highp vec2 delta;
                        varying highp vec2 qt_TexCoord0;
                        void main() {
                            gl_FragColor =(0.0538 * texture2D(source, qt_TexCoord0 - 3.182 * delta)
                                         + 0.3229 * texture2D(source, qt_TexCoord0 - 1.364 * delta)
                                         + 0.2466 * texture2D(source, qt_TexCoord0)
                                         + 0.3229 * texture2D(source, qt_TexCoord0 + 1.364 * delta)
                                         + 0.0538 * texture2D(source, qt_TexCoord0 + 3.182 * delta)) * qt_Opacity;
                        }"
                }
            }
            property real angle: 0
            property variant offset: Qt.point(15.0 * Math.cos(angle), 15.0 * Math.sin(angle))
            NumberAnimation on angle { loops: Animation.Infinite; from: 0; to: Math.PI * 2; duration: 6000 }
            property variant delta: Qt.size(offset.x / width, offset.y / height)
            property real darkness: shadowSlider.value
            fragmentShader: "
                uniform lowp float qt_Opacity;
                uniform highp vec2 offset;
                uniform sampler2D source;
                uniform sampler2D shadow;
                uniform highp float darkness;
                uniform highp vec2 delta;
                varying highp vec2 qt_TexCoord0;
                void main() {
                    lowp vec4 fg = texture2D(source, qt_TexCoord0);
                    lowp vec4 bg = texture2D(shadow, qt_TexCoord0 + delta);
                    gl_FragColor = (fg + vec4(0., 0., 0., darkness * bg.a) * (1. - fg.a)) * qt_Opacity;
                }"
            Slider {
                id: shadowSlider
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 40
            }
        }
        ShaderEffect {
            width: 160
            height: 160
            property variant source: theSource
            property variant delta: Qt.size(0.5 / width, 0.5 / height)
            fragmentShader: "
                uniform sampler2D source;
                uniform highp vec2 delta;
                uniform highp float qt_Opacity;
                varying highp vec2 qt_TexCoord0;
                void main() {
                    lowp vec4 tl = texture2D(source, qt_TexCoord0 - delta);
                    lowp vec4 tr = texture2D(source, qt_TexCoord0 + vec2(delta.x, -delta.y));
                    lowp vec4 bl = texture2D(source, qt_TexCoord0 - vec2(delta.x, -delta.y));
                    lowp vec4 br = texture2D(source, qt_TexCoord0 + delta);
                    mediump vec4 gx = (tl + bl) - (tr + br);
                    mediump vec4 gy = (tl + tr) - (bl + br);
                    gl_FragColor.xyz = vec3(0.);
                    gl_FragColor.w = clamp(dot(sqrt(gx * gx + gy * gy), vec4(1.)), 0., 1.) * qt_Opacity;
                }"
        }
        ShaderEffect {
            width: 160
            height: 160
            property variant source: theSource
            property color tint: root.sliderToColor(colorizeSlider.value)
            fragmentShader: "
                uniform sampler2D source;
                uniform lowp vec4 tint;
                uniform lowp float qt_Opacity;
                varying highp vec2 qt_TexCoord0;
                void main() {
                    lowp vec4 c = texture2D(source, qt_TexCoord0);
                    lowp float lo = min(min(c.x, c.y), c.z);
                    lowp float hi = max(max(c.x, c.y), c.z);
                    gl_FragColor = qt_Opacity * vec4(mix(vec3(lo), vec3(hi), tint.xyz), c.w);
                }"
            Slider {
                id: colorizeSlider
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 40
            }
        }
        ShaderEffect {
            width: 160
            height: 160
            //! [properties]
            property variant source: theSource
            property real bend: 0
            property real minimize: 0
            property real side: genieSlider.value
            SequentialAnimation on bend {
                loops: Animation.Infinite
                NumberAnimation { to: 1; duration: 700; easing.type: Easing.InOutSine }
                PauseAnimation { duration: 1600 }
                NumberAnimation { to: 0; duration: 700; easing.type: Easing.InOutSine }
                PauseAnimation { duration: 1000 }
            }
            SequentialAnimation on minimize {
                loops: Animation.Infinite
                PauseAnimation { duration: 300 }
                NumberAnimation { to: 1; duration: 700; easing.type: Easing.InOutSine }
                PauseAnimation { duration: 1000 }
                NumberAnimation { to: 0; duration: 700; easing.type: Easing.InOutSine }
                PauseAnimation { duration: 1300 }
            }
            //! [properties]
            //! [vertex]
            mesh: Qt.size(10, 10)
            vertexShader: "
                uniform highp mat4 qt_Matrix;
                uniform highp float bend;
                uniform highp float minimize;
                uniform highp float side;
                uniform highp float width;
                uniform highp float height;
                attribute highp vec4 qt_Vertex;
                attribute highp vec2 qt_MultiTexCoord0;
                varying highp vec2 qt_TexCoord0;
                void main() {
                    qt_TexCoord0 = qt_MultiTexCoord0;
                    highp vec4 pos = qt_Vertex;
                    pos.y = mix(qt_Vertex.y, height, minimize);
                    highp float t = pos.y / height;
                    t = (3. - 2. * t) * t * t;
                    pos.x = mix(qt_Vertex.x, side * width, t * bend);
                    gl_Position = qt_Matrix * pos;
                }"
            //! [vertex]
            Slider {
                id: genieSlider
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 40
            }
        }
    }
}
