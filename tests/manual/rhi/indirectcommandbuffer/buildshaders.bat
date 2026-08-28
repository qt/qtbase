:: Copyright (C) 2026 The Qt Company Ltd.
:: SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o quads.vert.qsb quads.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o quads.frag.qsb quads.frag
qsb --glsl "310 es,430" --hlsl 50 --msl 12 -o buildcmds.comp.qsb buildcmds.comp
