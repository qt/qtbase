/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef GRADIENTS_H
#define GRADIENTS_H

#include "arthurwidgets.h"

QT_BEGIN_NAMESPACE
class QRadioButton;
QT_END_NAMESPACE

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

    void paintEvent(QPaintEvent *e) Q_DECL_OVERRIDE;

    QSize sizeHint() const Q_DECL_OVERRIDE { return QSize(150, 40); }
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
    void paint(QPainter *p) Q_DECL_OVERRIDE;

    QSize sizeHint() const Q_DECL_OVERRIDE { return QSize(400, 400); }

    HoverPoints *hoverPoints() const { return m_hoverPoints; }
    void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;

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
