// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [2]
class View : public QGraphicsView
{
Q_OBJECT
    ...
public slots:
    void zoomIn() { scale(1.2, 1.2); }
    void zoomOut() { scale(1 / 1.2, 1 / 1.2); }
    void rotateLeft() { rotate(-10); }
    void rotateRight() { rotate(10); }
    ...
};
//! [2]
