qsb --glsl 320es,410 --hlsl 50 test.vert -o test.vert.qsb
qsb --glsl 320es,410 test.geom -o test.geom.qsb
qsb -r hlsl,50,test_geom.hlsl test.geom.qsb
qsb --glsl 320es,410 --hlsl 50 test.frag -o test.frag.qsb
