// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mesh.h"
#include <QtConcurrentRun>
#include <QFile>

void Mesh::load(const QString &fn)
{
    reset();
    m_maybeRunning = true;
    m_future = QtConcurrent::run([fn]() {
        MeshData md;
        QFile f(fn);
        if (!f.open(QIODevice::ReadOnly)) {
            qWarning("Failed to open %s", qPrintable(fn));
            return md;
        }
        QByteArray buf = f.readAll();
        const char *p = buf.constData();
        quint32 format;
        memcpy(&format, p, 4);
        if (format != 1) {
            qWarning("Invalid format in %s", qPrintable(fn));
            return md;
        }
        int ofs = 4;
        memcpy(&md.vertexCount, p + ofs, 4);
        ofs += 4;
        memcpy(md.aabb, p + ofs, 6 * 4);
        ofs += 6 * 4;
        const int byteCount = md.vertexCount * 8 * 4;
        md.geom.resize(byteCount);
        memcpy(md.geom.data(), p + ofs, byteCount);
        return md;
    });
}

MeshData *Mesh::data()
{
    if (m_maybeRunning && !m_data.isValid())
        m_data = m_future.result();

    return &m_data;
}

void Mesh::reset()
{
    *data() = MeshData();
    m_maybeRunning = false;
}
