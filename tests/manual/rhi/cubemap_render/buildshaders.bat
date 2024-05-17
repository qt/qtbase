:: Copyright (C) 2024 The Qt Company Ltd.
:: SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
qsb --glsl "300 es,120" --hlsl 50 --msl 12 cubemap_oneface.vert -o cubemap_oneface.vert.qsb
qsb --glsl "300 es,120" --hlsl 50 --msl 12 cubemap_oneface.frag -o cubemap_oneface.frag.qsb
qsb --glsl "300 es,120" --hlsl 50 --msl 12 cubemap_mrt.vert -o cubemap_mrt.vert.qsb
qsb --glsl "300 es,120" --hlsl 50 --msl 12 cubemap_mrt.frag -o cubemap_mrt.frag.qsb
qsb --glsl "300 es,120" --hlsl 50 --msl 12 cubemap_sample.vert -o cubemap_sample.vert.qsb
qsb --glsl "300 es,120" --hlsl 50 --msl 12 cubemap_sample.frag -o cubemap_sample.frag.qsb
