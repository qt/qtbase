// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MESH_H
#define MESH_H

#include <QString>
#include <QFuture>

struct MeshData
{
    bool isValid() const { return vertexCount > 0; }
    int vertexCount = 0;
    float aabb[6];
    QByteArray geom; // x, y, z, u, v, nx, ny, nz
};

class Mesh
{
public:
    void load(const QString &fn);
    MeshData *data();
    bool isValid() { return data()->isValid(); }
    void reset();

private:
    bool m_maybeRunning = false;
    QFuture<MeshData> m_future;
    MeshData m_data;
};

#endif
