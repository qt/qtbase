/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

varying vec3 position, normal;
varying vec4 specular, ambient, diffuse, lightDirection;

uniform sampler2D tex;
uniform samplerCube env;
uniform mat4 view;
uniform vec4 basicColor;

void main()
{
    vec3 N = normalize(normal);
    // assume directional light

    gl_MaterialParameters M = gl_FrontMaterial;

    float NdotL = dot(N, lightDirection.xyz);
    float RdotL = dot(reflect(normalize(position), N), lightDirection.xyz);

    vec3 absN = abs(gl_TexCoord[1].xyz);
    vec3 texCoord;
    if (absN.x > absN.y && absN.x > absN.z)
        texCoord = gl_TexCoord[1].yzx;
    else if (absN.y > absN.z)
        texCoord = gl_TexCoord[1].zxy;
    else
        texCoord = gl_TexCoord[1].xyz;
    texCoord.y *= -sign(texCoord.z);
    texCoord += 0.5;

    vec4 texColor = texture2D(tex, texCoord.xy);
    vec4 unlitColor = gl_Color * mix(basicColor, vec4(texColor.xyz, 1.0), texColor.w);
    vec4 litColor = (ambient + diffuse * max(NdotL, 0.0)) * unlitColor +
                     M.specular * specular * pow(max(RdotL, 0.0), M.shininess);

    vec3 R = 2.0 * dot(-position, N) * N + position;
    vec4 reflectedColor = textureCube(env, R * mat3(view[0].xyz, view[1].xyz, view[2].xyz));
    gl_FragColor = mix(litColor, reflectedColor, 0.2 + 0.8 * pow(1.0 + dot(N, normalize(position)), 2.0));
}
