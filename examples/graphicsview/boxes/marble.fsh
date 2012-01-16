/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

varying vec3 position, normal;
varying vec4 specular, ambient, diffuse, lightDirection;

uniform sampler2D tex;
uniform sampler3D noise;

//const vec4 marbleColors[2] = {vec4(0.9, 0.9, 0.9, 1), vec4(0.6, 0.5, 0.5, 1)};
uniform vec4 marbleColors[2];

void main()
{
    float turbulence = 0.0;
    float scale = 1.0;
    for (int i = 0; i < 4; ++i) {
        turbulence += scale * (texture3D(noise, 0.125 * gl_TexCoord[1].xyz / scale).x - 0.5);
        scale *= 0.5;
    }

    vec3 N = normalize(normal);
    // assume directional light

    gl_MaterialParameters M = gl_FrontMaterial;

    float NdotL = dot(N, lightDirection.xyz);
    float RdotL = dot(reflect(normalize(position), N), lightDirection.xyz);

    vec4 unlitColor = mix(marbleColors[0], marbleColors[1], exp(-4.0 * abs(turbulence)));
    gl_FragColor = (ambient + diffuse * max(NdotL, 0.0)) * unlitColor +
                    M.specular * specular * pow(max(RdotL, 0.0), M.shininess);
}
