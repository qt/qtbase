/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the demonstration applications of the Qt Toolkit.
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

#ifndef GRADIENTS_H
#define GRADIENTS_H

#include "arthurwidgets.h"

#include <QtGui>

class HoverPoints;


class ShadeWidget : public QWidget
{
    Q_OBJECT
public:
    enum ShadeType {
        RedShade,
        GreenShade,
        BlueShade,
        ARGBShade
    };

    ShadeWidget(ShadeType type, QWidget *parent);

    void setGradientStops(const QGradientStops &stops);

    void paintEvent(QPaintEvent *e);

    QSize sizeHint() const { return QSize(150, 40); }
    QPolygonF points() const;

    HoverPoints *hoverPoints() const { return m_hoverPoints; }

    uint colorAt(int x);

signals:
    void colorsChanged();

private:
    void generateShade();

    ShadeType m_shade_type;
    QImage m_shade;
    HoverPoints *m_hoverPoints;
    QLinearGradient m_alpha_gradient;
};

class GradientEditor : public QWidget
{
    Q_OBJECT
public:
    GradientEditor(QWidget *parent);

    void setGradientStops(const QGradientStops &stops);

public slots:
    void pointsUpdated();

signals:
    void gradientStopsChanged(const QGradientStops &stops);

private:
    ShadeWidget *m_red_shade;
    ShadeWidget *m_green_shade;
    ShadeWidget *m_blue_shade;
    ShadeWidget *m_alpha_shade;
};


class GradientRenderer : public ArthurFrame
{
    Q_OBJECT
public:
    GradientRenderer(QWidget *parent);
    void paint(QPainter *p);

    QSize sizeHint() const { return QSize(400, 400); }

    HoverPoints *hoverPoints() const { return m_hoverPoints; }
    void mousePressEvent(QMouseEvent *e);

public slots:
    void setGradientStops(const QGradientStops &stops);

    void setPadSpread() { m_spread = QGradient::PadSpread; update(); }
    void setRepeatSpread() { m_spread = QGradient::RepeatSpread; update(); }
    void setReflectSpread() { m_spread = QGradient::ReflectSpread; update(); }

    void setLinearGradient() { m_gradientType = Qt::LinearGradientPattern; update(); }
    void setRadialGradient() { m_gradientType = Qt::RadialGradientPattern; update(); }
    void setConicalGradient() { m_gradientType = Qt::ConicalGradientPattern; update(); }


private:
    QGradientStops m_stops;
    HoverPoints *m_hoverPoints;

    QGradient::Spread m_spread;
    Qt::BrushStyle m_gradientType;
};


class GradientWidget : public QWidget
{
    Q_OBJECT
public:
    GradientWidget(QWidget *parent);

public slots:
    void setDefault1() { setDefault(1); }
    void setDefault2() { setDefault(2); }
    void setDefault3() { setDefault(3); }
    void setDefault4() { setDefault(4); }

private:
    void setDefault(int i);

    GradientRenderer *m_renderer;
    GradientEditor *m_editor;

    QRadioButton *m_linearButton;
    QRadioButton *m_radialButton;
    QRadioButton *m_conicalButton;
    QRadioButton *m_padSpreadButton;
    QRadioButton *m_reflectSpreadButton;
    QRadioButton *m_repeatSpreadButton;

};

#endif // GRADIENTS_H
