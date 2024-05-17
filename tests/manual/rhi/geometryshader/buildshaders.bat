:: Copyright (C) 2024 The Qt Company Ltd.
:: SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
qsb --glsl 320es,410 --hlsl 50 test.vert -o test.vert.qsb
qsb --glsl 320es,410 test.geom -o test.geom.qsb
qsb -r hlsl,50,test_geom.hlsl test.geom.qsb
qsb --glsl 320es,410 --hlsl 50 test.frag -o test.frag.qsb
