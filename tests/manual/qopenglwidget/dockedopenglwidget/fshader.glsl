// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
uniform sampler2D texture;
varying vec2 v_texcoord;

void main()
{
    gl_FragColor = texture2D(texture, v_texcoord);
}

