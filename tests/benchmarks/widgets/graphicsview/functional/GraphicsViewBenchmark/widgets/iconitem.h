// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ICONITEM_H
#define ICONITEM_H

#include <QPainter>

#include "gvbwidget.h"

class QGraphicsOpacityEffect;
class QPainter;

class IconItem : public GvbWidget
{
    Q_OBJECT

public:

    IconItem(const QString &filename = "", QGraphicsItem *parent = nullptr);

    virtual ~IconItem();

    QString fileName() const;
    void setFileName(const QString &filename);

    void setOpacityEffectEnabled(const bool enable);
    bool isOpacityEffectEnabled() const;

    void setRotation(const qreal rotation) { m_rotation = rotation; }
    qreal rotation() const { return m_rotation; }

    void setSmoothTransformationEnabled(const bool enable) { m_smoothTransformation = enable; }
    bool isSmoothTransformationEnabled() const { return m_smoothTransformation; }

private:

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget */*widget = 0*/);
    QSizeF sizeHint(Qt::SizeHint which,
        const QSizeF &constraint = QSizeF()) const;

private:
    Q_DISABLE_COPY(IconItem)
    void reload();

    QString m_filename;
    QPixmap m_pixmap;
    qreal m_rotation;
    QGraphicsOpacityEffect *m_opacityEffect;
    bool m_smoothTransformation;
};

#endif
