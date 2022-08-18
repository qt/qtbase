:: Copyright (C) 2019 The Qt Company Ltd.
:: SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

qsb --glsl "150,120,100 es" --hlsl 50 -c --msl 12 -o simple.vert.qsb simple.vert
qsb --glsl "150,120,100 es" --hlsl 50 -c --msl 12 -o simple.frag.qsb simple.frag
qsb --glsl "150,120,100 es" --hlsl 50 -c --msl 12 -o simpletextured.vert.qsb simpletextured.vert
qsb --glsl "150,120,100 es" --hlsl 50 -c --msl 12 -o simpletextured.frag.qsb simpletextured.frag
qsb --glsl "150,120,100 es" --hlsl 50 -c --msl 20 -o simpletextured_array.frag.qsb simpletextured_array.frag
qsb --glsl "150,120,100 es" --hlsl 50 -c --msl 12 -o simpletextured_separate.frag.qsb simpletextured_separate.frag
qsb --glsl "150,120,100 es" --hlsl 50 -c --msl 12 -o textured.vert.qsb textured.vert
qsb --glsl "150,120,100 es" --hlsl 50 -c --msl 12 -o textured.frag.qsb textured.frag
qsb --glsl "150,120,100 es" --hlsl 50 -c --msl 12 -o textured_multiubuf.vert.qsb textured_multiubuf.vert
qsb --glsl "150,120,100 es" --hlsl 50 -c --msl 12 -o textured_multiubuf.frag.qsb textured_multiubuf.frag
qsb --glsl 320es,410 --msl 12 --msltess simpletess.vert -o simpletess.vert.qsb
qsb --glsl 320es,410 --msl 12 --tess-mode triangles simpletess.tesc -o simpletess.tesc.qsb
qsb --glsl 320es,410 --msl 12  --tess-vertex-count 3 simpletess.tese -o simpletess.tese.qsb
qsb --glsl 320es,410 --msl 12 simpletess.frag -o simpletess.frag.qsb
