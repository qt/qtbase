qsb --glsl 320es,410 --hlsl 50 --msl 12 --msltess test.vert -o test.vert.qsb
qsb --glsl 320es,410 --msl 12 --tess-mode triangles test.tesc -o test.tesc.qsb
qsb -r hlsl,50,test_hull.hlsl test.tesc.qsb
qsb --glsl 320es,410 --msl 12  --tess-vertex-count 3 test.tese -o test.tese.qsb
qsb -r hlsl,50,test_domain.hlsl test.tese.qsb
qsb --glsl 320es,410 --hlsl 50 --msl 12 test.frag -o test.frag.qsb
