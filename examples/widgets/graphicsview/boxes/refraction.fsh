/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
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
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

varying vec3 position, normal;
varying vec4 specular, ambient, diffuse, lightDirection;

uniform sampler2D tex;
uniform samplerCube env;
uniform mat4 view;

// Arrays don't work here on glsl < 120, apparently.
//const float coeffs[6] = float[6](1.0/2.0, 1.0/2.1, 1.0/2.2, 1.0/2.3, 1.0/2.4, 1.0/2.5);
float coeffs(int i)
{
	return 1.0 / (2.0 + 0.1 * float(i));
}

void main()
{
    vec3 N = normalize(normal);
    vec3 I = -normalize(position);
    float IdotN = dot(I, N);
    float scales[6];
    vec3 C[6];
    for (int i = 0; i < 6; ++i) {
        scales[i] = (IdotN - sqrt(1.0 - coeffs(i) + coeffs(i) * (IdotN * IdotN)));
        C[i] = textureCube(env, (-I + coeffs(i) * N) * mat3(view[0].xyz, view[1].xyz, view[2].xyz)).xyz;
    }

    gl_FragColor = 0.25 * vec4(C[5].x + 2.0*C[0].x + C[1].x, C[1].y + 2.0*C[2].y + C[3].y,
                   C[3].z + 2.0*C[4].z + C[5].z, 4.0);
}
