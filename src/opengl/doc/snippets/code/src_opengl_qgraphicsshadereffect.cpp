/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//! [0]
    static char const colorizeShaderCode[] =
        "uniform lowp vec4 effectColor;\n"
        "lowp vec4 customShader(lowp sampler2D imageTexture, highp vec2 textureCoords) {\n"
        "    vec4 src = texture2D(imageTexture, textureCoords);\n"
        "    float gray = dot(src.rgb, vec3(0.212671, 0.715160, 0.072169));\n"
        "    vec4 colorize = 1.0-((1.0-gray)*(1.0-effectColor));\n"
        "    return vec4(colorize.rgb, src.a);\n"
        "}";
//! [0]

//! [1]
    class ColorizeEffect : public QGraphicsShaderEffect
    {
        Q_OBJECT
    public:
        ColorizeEffect(QObject *parent = 0)
            : QGraphicsShaderEffect(parent), color(Qt::black)
        {
            setPixelShaderFragment(colorizeShaderCode);
        }

        QColor effectColor() const { return color; }
        void setEffectColor(const QColor& c)
        {
            color = c;
            setUniformsDirty();
        }

    protected:
        void setUniforms(QGLShaderProgram *program)
        {
            program->setUniformValue("effectColor", color);
        }

    private:
        QColor color;
    };
//! [1]

//! [2]
    lowp vec4 customShader(lowp sampler2D imageTexture, highp vec2 textureCoords) {
        return texture2D(imageTexture, textureCoords);
    }
//! [2]
