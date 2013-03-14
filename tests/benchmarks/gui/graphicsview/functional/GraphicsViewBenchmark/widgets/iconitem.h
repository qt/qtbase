/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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
