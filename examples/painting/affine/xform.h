/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef XFORM_H
#define XFORM_H

#include "arthurwidgets.h"

#include <QBasicTimer>
#include <QPolygonF>

class HoverPoints;
QT_FORWARD_DECLARE_CLASS(QLineEdit)

class XFormView : public ArthurFrame
{
public:
    Q_OBJECT

    Q_PROPERTY(XFormType type READ type WRITE setType)
    Q_PROPERTY(bool animation READ animation WRITE setAnimation)
    Q_PROPERTY(qreal shear READ shear WRITE setShear)
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation)
    Q_PROPERTY(qreal scale READ scale WRITE setScale)
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QPixmap pixmap READ pixmap WRITE setPixmap)
    Q_ENUMS(XFormType)

public:
    enum XFormType { VectorType, PixmapType, TextType };

    XFormView(QWidget *parent);
    void paint(QPainter *);
    void drawVectorType(QPainter *painter);
    void drawPixmapType(QPainter *painter);
    void drawTextType(QPainter *painter);
    QSize sizeHint() const { return QSize(500, 500); }

    void mousePressEvent(QMouseEvent *e);
    void resizeEvent(QResizeEvent *e);
    HoverPoints *hoverPoints() { return pts; }

    bool animation() const { return timer.isActive(); }
    qreal shear() const { return m_shear; }
    qreal scale() const { return m_scale; }
    qreal rotation() const { return m_rotation; }
    void setShear(qreal s);
    void setScale(qreal s);
    void setRotation(qreal r);

    XFormType type() const;
    QPixmap pixmap() const;
    QString text() const;

public slots:
    void setAnimation(bool animate);
    void updateCtrlPoints(const QPolygonF &);
    void changeRotation(int rotation);
    void changeScale(int scale);
    void changeShear(int shear);

    void setText(const QString &);
    void setPixmap(const QPixmap &);
    void setType(XFormType t);

    void setVectorType();
    void setPixmapType();
    void setTextType();
    void reset();

signals:
    void rotationChanged(int rotation);
    void scaleChanged(int scale);
    void shearChanged(int shear);

protected:
    void timerEvent(QTimerEvent *e);
    void wheelEvent(QWheelEvent *);

private:
    QPolygonF ctrlPoints;
    HoverPoints *pts;
    qreal m_rotation;
    qreal m_scale;
    qreal m_shear;
    XFormType m_type;
    QPixmap m_pixmap;
    QString m_text;
    QBasicTimer timer;
};

class XFormWidget : public QWidget
{
    Q_OBJECT
public:
    XFormWidget(QWidget *parent);

private:
    XFormView *view;
    QLineEdit *textEditor;
};

#endif // XFORM_H
