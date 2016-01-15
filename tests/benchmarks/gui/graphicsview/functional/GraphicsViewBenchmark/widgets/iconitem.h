/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

    IconItem(const QString &filename = "", QGraphicsItem *parent = 0);

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
