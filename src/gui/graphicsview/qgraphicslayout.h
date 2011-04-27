/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGRAPHICSLAYOUT_H
#define QGRAPHICSLAYOUT_H

#include <QtGui/qgraphicslayoutitem.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#if !defined(QT_NO_GRAPHICSVIEW) || (QT_EDITION & QT_MODULE_GRAPHICSVIEW) != QT_MODULE_GRAPHICSVIEW

class QGraphicsLayoutPrivate;
class QGraphicsLayoutItem;
class QGraphicsWidget;

class Q_GUI_EXPORT QGraphicsLayout : public QGraphicsLayoutItem
{
public:
    QGraphicsLayout(QGraphicsLayoutItem *parent = 0);
    ~QGraphicsLayout();

    void setContentsMargins(qreal left, qreal top, qreal right, qreal bottom);
    void getContentsMargins(qreal *left, qreal *top, qreal *right, qreal *bottom) const;

    void activate();
    bool isActivated() const;
    virtual void invalidate();
    virtual void updateGeometry();

    virtual void widgetEvent(QEvent *e);

    virtual int count() const = 0;
    virtual QGraphicsLayoutItem *itemAt(int i) const = 0;
    virtual void removeAt(int index) = 0;

protected:
    QGraphicsLayout(QGraphicsLayoutPrivate &, QGraphicsLayoutItem *);
    void addChildLayoutItem(QGraphicsLayoutItem *layoutItem);

private:
    Q_DISABLE_COPY(QGraphicsLayout)
    Q_DECLARE_PRIVATE(QGraphicsLayout)
    friend class QGraphicsWidget;
};

Q_DECLARE_INTERFACE(QGraphicsLayout, "com.trolltech.Qt.QGraphicsLayout")

#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif

