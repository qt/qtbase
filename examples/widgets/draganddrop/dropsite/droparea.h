// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DROPAREA_H
#define DROPAREA_H

#include <QLabel>

QT_BEGIN_NAMESPACE
class QMimeData;
QT_END_NAMESPACE

//! [DropArea header part1]
class DropArea : public QLabel
{
    Q_OBJECT

public:
    explicit DropArea(QWidget *parent = nullptr);

public slots:
    void clear();

signals:
    void changed(const QMimeData *mimeData = nullptr);
//! [DropArea header part1]

//! [DropArea header part2]
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
};
//! [DropArea header part2]

#endif // DROPAREA_H
