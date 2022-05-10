// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef PIXELDELEGATE_H
#define PIXELDELEGATE_H

#include <QAbstractItemDelegate>
#include <QModelIndex>
#include <QSize>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QObject;
class QPainter;
QT_END_NAMESPACE

static constexpr int ItemSize = 256;

//! [0]
class PixelDelegate : public QAbstractItemDelegate
{
    Q_OBJECT

public:
    PixelDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

public slots:
    void setPixelSize(int size);

private:
    int pixelSize;
};
//! [0]

#endif // PIXELDELEGATE_H
