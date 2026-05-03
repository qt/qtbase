:: Copyright (C) 2026 The Qt Company Ltd.
:: SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
qsb --glsl "310 es,430" --hlsl 50 --msl 12 dispatch_args.comp -o dispatch_args.comp.qsb
qsb --glsl "310 es,430" --hlsl 50 --msl 12 dispatch_consume.comp -o dispatch_consume.comp.qsb
qsb --glsl "310 es,430" --hlsl 50 --msl 12 dispatch_reset.comp -o dispatch_reset.comp.qsb
qsb --glsl "310 es,430" --hlsl 50 --msl 12 main.vert -o main.vert.qsb
qsb --glsl "310 es,430" --hlsl 50 --msl 12 main.frag -o main.frag.qsb
