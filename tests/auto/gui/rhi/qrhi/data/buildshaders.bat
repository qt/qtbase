:: Copyright (C) 2019 The Qt Company Ltd.
:: SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
qsb --glsl 320es,400 --hlsl 50 -c --msl 12 -o mrtbl.vert.qsb mrtbl.vert
qsb --glsl 320es,400 --hlsl 50 -c --msl 12 -o mrtbl.frag.qsb mrtbl.frag
qsb --glsl 320es,410 --msl 12 --msltess simpletess.vert -o simpletess.vert.qsb
qsb --glsl 320es,410 --msl 12 --tess-mode triangles simpletess.tesc -o simpletess.tesc.qsb
qsb --glsl 320es,410 --msl 12  --tess-vertex-count 3 simpletess.tese -o simpletess.tese.qsb
qsb --glsl 320es,410 --msl 12 simpletess.frag -o simpletess.frag.qsb
qsb --glsl 310es,430 --msl 12 --hlsl 50 storagebuffer.comp -o storagebuffer.comp.qsb
qsb --glsl 320es,430 --msl 12 --msltess storagebuffer_runtime.vert -o storagebuffer_runtime.vert.qsb
qsb --glsl 320es,430 --msl 12 --tess-mode triangles storagebuffer_runtime.tesc -o storagebuffer_runtime.tesc.qsb
qsb --glsl 320es,430 --msl 12 --tess-vertex-count 3 storagebuffer_runtime.tese -o storagebuffer_runtime.tese.qsb
qsb --glsl 320es,430 --msl 12 storagebuffer_runtime.frag -o storagebuffer_runtime.frag.qsb
qsb --glsl 320es,430 --hlsl 50 -c --msl 12 storagebuffer_runtime.comp -o storagebuffer_runtime.comp.qsb
qsb --glsl "150,120,100 es" --hlsl 50 -c --msl 12 -o half.vert.qsb half.vert
qsb --glsl 320es,430 --msl 21 --msltess tessinterfaceblocks.vert -o tessinterfaceblocks.vert.qsb
qsb --glsl 320es,430 --msl 21 --tess-mode triangles tessinterfaceblocks.tesc -o tessinterfaceblocks.tesc.qsb
qsb --glsl 320es,430 --msl 21  --tess-vertex-count 3 tessinterfaceblocks.tese -o tessinterfaceblocks.tese.qsb
qsb --glsl 320es,430 --msl 21 simpletess.frag -o tessinterfaceblocks.frag.qsb
qsb --view-count 2 --glsl "300 es,330" --hlsl 61 -c --msl 12 multiview.vert -o multiview.vert.qsb
qsb --glsl "300 es,330" --hlsl 61 -c --msl 12 multiview.frag -o multiview.frag.qsb
