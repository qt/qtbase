/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
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
