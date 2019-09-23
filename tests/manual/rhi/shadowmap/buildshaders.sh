#!/bin/sh
qsb --glsl "120,300 es" --hlsl 50 --msl 12 shadowmap.vert -o shadowmap.vert.qsb
qsb --glsl "120,300 es" --hlsl 50 --msl 12 shadowmap.frag -o shadowmap.frag.qsb
qsb --glsl "120,300 es" --hlsl 50 --msl 12 main.vert -o main.vert.qsb
qsb --glsl "120,300 es" --hlsl 50 --msl 12 main.frag -o main.frag.qsb
