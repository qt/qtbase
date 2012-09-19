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

// Some arbitrary values
// Arrays don't work here on glsl < 120, apparently.
//const float coeffs[6] = float[6](1.0/4.0, 1.0/4.1, 1.0/4.2, 1.0/4.3, 1.0/4.4, 1.0/4.5);
float coeffs(int i)
{
	return 1.0 / (3.0 + 0.1 * float(i));
}

void main()
{
    vec3 N = normalize(normal);
    vec3 I = -normalize(position);
    mat3 V = mat3(view[0].xyz, view[1].xyz, view[2].xyz);
    float IdotN = dot(I, N);
    float scales[6];
    vec3 C[6];
    for (int i = 0; i < 6; ++i) {
        scales[i] = (IdotN - sqrt(1.0 - coeffs(i) + coeffs(i) * (IdotN * IdotN)));
        C[i] = textureCube(env, (-I + coeffs(i) * N) * V).xyz;
    }
    vec4 refractedColor = 0.25 * vec4(C[5].x + 2.0*C[0].x + C[1].x, C[1].y + 2.0*C[2].y + C[3].y,
                          C[3].z + 2.0*C[4].z + C[5].z, 4.0);

    vec3 R = 2.0 * dot(-position, N) * N + position;
    vec4 reflectedColor = textureCube(env, R * V);

    gl_FragColor = mix(refractedColor, reflectedColor, 0.4 + 0.6 * pow(1.0 - IdotN, 2.0));
}
