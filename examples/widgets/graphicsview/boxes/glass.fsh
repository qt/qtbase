/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
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
