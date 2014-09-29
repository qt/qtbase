/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
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
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef COMPOSITION_H
#define COMPOSITION_H

#include "arthurwidgets.h"

#include <QPainter>
#include <QEvent>

QT_BEGIN_NAMESPACE
class QPushButton;
class QRadioButton;
QT_END_NAMESPACE

#ifdef QT_OPENGL_SUPPORT
#include <QtOpenGL>
#endif

class CompositionWidget : public QWidget
{
    Q_OBJECT

public:
    CompositionWidget(QWidget *parent);

public slots:
void nextMode();

private:
    bool m_cycle_enabled;

    QRadioButton *rbClear;
    QRadioButton *rbSource;
    QRadioButton *rbDest;
    QRadioButton *rbSourceOver;
    QRadioButton *rbDestOver;
    QRadioButton *rbSourceIn;
    QRadioButton *rbDestIn;
    QRadioButton *rbSourceOut;
    QRadioButton *rbDestOut;
    QRadioButton *rbSourceAtop;
    QRadioButton *rbDestAtop;
    QRadioButton *rbXor;

    QRadioButton *rbPlus;
    QRadioButton *rbMultiply;
    QRadioButton *rbScreen;
    QRadioButton *rbOverlay;
    QRadioButton *rbDarken;
    QRadioButton *rbLighten;
    QRadioButton *rbColorDodge;
    QRadioButton *rbColorBurn;
    QRadioButton *rbHardLight;
    QRadioButton *rbSoftLight;
    QRadioButton *rbDifference;
    QRadioButton *rbExclusion;
};

class CompositionRenderer : public ArthurFrame
{
    Q_OBJECT

    enum ObjectType { NoObject, Circle, Rectangle, Image };

    Q_PROPERTY(int circleColor READ circleColor WRITE setCircleColor)
    Q_PROPERTY(int circleAlpha READ circleAlpha WRITE setCircleAlpha)
    Q_PROPERTY(bool animation READ animationEnabled WRITE setAnimationEnabled)

public:
    CompositionRenderer(QWidget *parent);

    void paint(QPainter *) Q_DECL_OVERRIDE;

    void setCirclePos(const QPointF &pos);

    QSize sizeHint() const Q_DECL_OVERRIDE { return QSize(500, 400); }

    bool animationEnabled() const { return m_animation_enabled; }
    int circleColor() const { return m_circle_hue; }
    int circleAlpha() const { return m_circle_alpha; }

protected:
    void mousePressEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void timerEvent(QTimerEvent *) Q_DECL_OVERRIDE;

public slots:
    void setClearMode() { m_composition_mode = QPainter::CompositionMode_Clear; update(); }
    void setSourceMode() { m_composition_mode = QPainter::CompositionMode_Source; update(); }
    void setDestMode() { m_composition_mode = QPainter::CompositionMode_Destination; update(); }
    void setSourceOverMode() { m_composition_mode = QPainter::CompositionMode_SourceOver; update(); }
    void setDestOverMode() { m_composition_mode = QPainter::CompositionMode_DestinationOver; update(); }
    void setSourceInMode() { m_composition_mode = QPainter::CompositionMode_SourceIn; update(); }
    void setDestInMode() { m_composition_mode = QPainter::CompositionMode_DestinationIn; update(); }
    void setSourceOutMode() { m_composition_mode = QPainter::CompositionMode_SourceOut; update(); }
    void setDestOutMode() { m_composition_mode = QPainter::CompositionMode_DestinationOut; update(); }
    void setSourceAtopMode() { m_composition_mode = QPainter::CompositionMode_SourceAtop; update(); }
    void setDestAtopMode() { m_composition_mode = QPainter::CompositionMode_DestinationAtop; update(); }
    void setXorMode() { m_composition_mode = QPainter::CompositionMode_Xor; update(); }

    void setPlusMode() { m_composition_mode = QPainter::CompositionMode_Plus; update(); }
    void setMultiplyMode() { m_composition_mode = QPainter::CompositionMode_Multiply; update(); }
    void setScreenMode() { m_composition_mode = QPainter::CompositionMode_Screen; update(); }
    void setOverlayMode() { m_composition_mode = QPainter::CompositionMode_Overlay; update(); }
    void setDarkenMode() { m_composition_mode = QPainter::CompositionMode_Darken; update(); }
    void setLightenMode() { m_composition_mode = QPainter::CompositionMode_Lighten; update(); }
    void setColorDodgeMode() { m_composition_mode = QPainter::CompositionMode_ColorDodge; update(); }
    void setColorBurnMode() { m_composition_mode = QPainter::CompositionMode_ColorBurn; update(); }
    void setHardLightMode() { m_composition_mode = QPainter::CompositionMode_HardLight; update(); }
    void setSoftLightMode() { m_composition_mode = QPainter::CompositionMode_SoftLight; update(); }
    void setDifferenceMode() { m_composition_mode = QPainter::CompositionMode_Difference; update(); }
    void setExclusionMode() { m_composition_mode = QPainter::CompositionMode_Exclusion; update(); }

    void setCircleAlpha(int alpha) { m_circle_alpha = alpha; update(); }
    void setCircleColor(int hue) { m_circle_hue = hue; update(); }
    void setAnimationEnabled(bool enabled);

private:
    void updateCirclePos();
    void drawBase(QPainter &p);
    void drawSource(QPainter &p);

    QPainter::CompositionMode m_composition_mode;

    QImage m_image;
    QImage m_buffer;
    QImage m_base_buffer;

    int m_circle_alpha;
    int m_circle_hue;

    QPointF m_circle_pos;
    QPointF m_offset;

    ObjectType m_current_object;
    bool m_animation_enabled;
    int m_animationTimer;

#ifdef QT_OPENGL_SUPPORT
    QGLPixelBuffer *m_pbuffer;
    GLuint m_base_tex;
    GLuint m_compositing_tex;
    int m_pbuffer_size; // width==height==size of pbuffer
    QSize m_previous_size;
#endif
};

#endif // COMPOSITION_H
