#!/bin/sh
qsb --glsl "430,310 es" --hlsl 50 --msl 12 load.comp -o load.comp.qsb
qsb --glsl "430,310 es" --hlsl 50 --msl 12 prefilter.comp -o prefilter.comp.qsb
