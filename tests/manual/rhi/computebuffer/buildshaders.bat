:: Copyright (C) 2024 The Qt Company Ltd.
:: SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
qsb --glsl "310 es,430" --hlsl 50 --msl 12 buffer.comp -o buffer.comp.qsb
qsb --glsl "310 es,430" --hlsl 50 --msl 12 main.vert -o main.vert.qsb
qsb --glsl "310 es,430" --hlsl 50 --msl 12 main.frag -o main.frag.qsb
