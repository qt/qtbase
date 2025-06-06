// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef NAME_COLLISION_H
#define NAME_COLLISION_H

#include <QObject>

namespace myns {

class NameCollision : public QObject
{
    Q_OBJECT

    // intentionally not fully qualified
    Q_PROPERTY(Status Status READ Status WRITE setStatus)

    int m_status = 0;

public:
    enum Status {};

    void statusChanged(Status status);

public Q_SLOTS:
    void setStatus(Status ) {}
    Status Status() { return {}; }
};

}


#endif // TECH_PREVIEW_H
