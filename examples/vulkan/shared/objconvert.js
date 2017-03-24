/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

var fs = require('fs');

var metadata = {
    vertexCount: 0,
    aabb: [[null, null], [null, null], [null, null]],
    emitVertex: function(v) {
        ++metadata.vertexCount;
        var aabb = metadata.aabb;
        if (aabb[0][0] === null || v[0] < aabb[0][0]) // min x
            aabb[0][0] = v[0];
        if (aabb[0][1] === null || v[0] > aabb[0][1]) // max x
            aabb[0][1] = v[0];
        if (aabb[1][0] === null || v[1] < aabb[1][0]) // min y
            aabb[1][0] = v[1];
        if (aabb[1][1] === null || v[1] > aabb[1][1]) // max y
            aabb[1][1] = v[1];
        if (aabb[2][0] === null || v[2] < aabb[2][0]) // min z
            aabb[2][0] = v[2];
        if (aabb[2][1] === null || v[2] > aabb[2][1]) // max z
            aabb[2][1] = v[2];
    },
    getBuffer: function() {
        var aabb = metadata.aabb;
        console.log(metadata.vertexCount + " vertices");
        console.log("AABB: " + aabb[0][0] + ".." + aabb[0][1]
                    + ", " + aabb[1][0] + ".." + aabb[1][1]
                    + ", " + aabb[2][0] + ".." + aabb[2][1]);
        var buf = new Buffer((2 + 6) * 4);
        var format = 1, p = 0;
        buf.writeUInt32LE(format, p++);
        buf.writeUInt32LE(metadata.vertexCount, p++ * 4);
        for (var i = 0; i < 3; ++i) {
            buf.writeFloatLE(aabb[i][0], p++ * 4);
            buf.writeFloatLE(aabb[i][1], p++ * 4);
        }
        return buf;
    }
};

function makeVec(s, n) {
    var v = [];
    s.split(' ').forEach(function (coordStr) {
        var coord = parseFloat(coordStr);
        if (!isNaN(coord))
            v.push(coord);
    });
    if (v.length != n) {
        console.error("Wrong vector size, expected " + n + ", got " + v.length);
        process.exit();
    }
    return v;
}

function parseObj(filename, callback) {
    fs.readFile(filename, "ascii", function (err, data) {
        if (err)
            throw err;
        var groupCount = 0;
        var parsed = { 'vertices': [], 'normals': [], 'texcoords': [], 'links': [] };
        var missingTexCount = 0, missingNormCount = 0;
        data.split('\n').forEach(function (line) {
            var s = line.trim();
            if (!s.length || groupCount > 1)
                return;
            if (s[0] === '#')
                return;
            if (s[0] === 'g') {
                ++groupCount;
            } else if (s.substr(0, 2) === "v ") {
                parsed.vertices.push(makeVec(s, 3));
            } else if (s.substr(0, 3) === "vn ") {
                parsed.normals.push(makeVec(s, 3));
            } else if (s.substr(0, 3) === "vt ") {
                parsed.texcoords.push(makeVec(s, 2));
            } else if (s.substr(0, 2) === "f ") {
                var refs = s.split(' ');
                var vertCount = refs.length - 1;
                if (vertCount != 3)
                    console.warn("Face " + parsed.links.length / 3 + " has " + vertCount + " vertices! (not triangulated?)");
                for (var i = 1, ie = Math.min(4, refs.length); i < ie; ++i) {
                    var refComps = refs[i].split('/');
                    var vertIndex = parseInt(refComps[0]) - 1;
                    var texIndex = -1;
                    if (refComps.length >= 2 && refComps[1].length)
                        texIndex = parseInt(refComps[1]) - 1;
                    var normIndex = -1;
                    if (refComps.length >= 3 && refComps[2].length)
                        normIndex = parseInt(refComps[2]) - 1;
                    parsed.links.push([vertIndex, texIndex, normIndex]);
                    if (texIndex == -1)
                        ++missingTexCount;
                    if (normIndex == -1)
                        ++missingNormCount;
                }
            }
        });
        console.log(missingTexCount + " missing texture coordinates, " + missingNormCount + " missing normals");
        callback(parsed);
    });
}

function fillVert(src, index, dst, elemCount, isVertexCoord) {
    var vertex = [];
    if (index >= 0) {
        for (var i = 0; i < elemCount; ++i) {
            var elem = src[index][i];
            if (isVertexCoord)
                vertex.push(elem);
            dst.buf.writeFloatLE(elem, dst.bufptr++ * 4);
        }
        if (vertex.length == 3)
            metadata.emitVertex(vertex);
    } else {
        if (isVertexCoord) {
            console.error("Missing vertex");
            process.exit();
        }
        for (var i = 0; i < elemCount; ++i)
            dst.buf.writeFloatLE(0, dst.bufptr++ * 4);
    }
    return vertex;
}

function normalize(v) {
    var len = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    if (len == 0.0 || len == 1.0)
        return;
    len = Math.sqrt(len);
    return [ v[0] / len, v[1] / len, v[2] / len ];
}

function surfaceNormal(a, b, c) {
    var u = [ b[0] - a[0], b[1] - a[1], b[2] - a[2] ];
    var v = [ c[0] - a[0], c[1] - a[1], c[2] - a[2] ];
    var result = [ u[1] * v[2] - u[2] * v[1],
                   u[2] * v[0] - u[0] * v[2],
                   u[0] * v[1] - u[1] * v[0] ];
    return normalize(result);
}

function objDataToBuf(parsed) {
    var floatCount = parsed.links.length * (3 + 2 + 3);
    var buf = new Buffer(floatCount * 4);
    var dst = { 'buf': buf, 'bufptr': 0 };
    var tri = [];
    var genNormals = false;
    var genNormCount = 0;
    for (var i = 0; i < parsed.links.length; ++i) {
        var link = parsed.links[i];
        var vertIndex = link[0], texIndex = link[1], normIndex = link[2];
        tri.push(fillVert(parsed.vertices, vertIndex, dst, 3, true));
        fillVert(parsed.texcoords, texIndex, dst, 2);
        fillVert(parsed.normals, normIndex, dst, 3);
        if (normIndex == -1)
            genNormals = true;
        if (tri.length == 3) {
            if (genNormals) {
                var norm = surfaceNormal(tri[0], tri[1], tri[2]);
                for (var nvIdx = 0; nvIdx < 3; ++nvIdx) {
                    dst.buf.writeFloatLE(norm[0], (dst.bufptr - 3 - nvIdx * 8) * 4);
                    dst.buf.writeFloatLE(norm[1], (dst.bufptr - 2 - nvIdx * 8) * 4);
                    dst.buf.writeFloatLE(norm[2], (dst.bufptr - 1 - nvIdx * 8) * 4);
                }
                genNormCount += 3;
            }
            tri = [];
        }
    }
    if (genNormCount)
        console.log("Generated " + genNormCount + " normals");
    return buf;
}

var inFilename = process.argv[2];
var outFilename = process.argv[3];

if (process.argv.length < 4) {
    console.log("Usage: objconvert file.obj file.buf");
    process.exit();
}

parseObj(inFilename, function (parsed) {
    var buf = objDataToBuf(parsed);
    var f = fs.createWriteStream(outFilename);
    f.on("error", function (e) { console.error(e); });
    f.write(metadata.getBuffer());
    f.write(buf);
    f.end();
    console.log("Written to " + outFilename + ", format is:");
    console.log("  uint32 version, uint32 vertex_count, float32 aabb[6], vertex_count * (float32 vertex[3], float32 texcoord[2], float32 normal[3])");
});
