// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef COMPOSITION_H
#define COMPOSITION_H

#include "arthurwidgets.h"

#if QT_CONFIG(opengl)
#include "fbopaintdevice.h"
#include <QOpenGLTextureBlitter>
#endif

#include <QPainter>
#include <QEvent>

#include <memory>

QT_BEGIN_NAMESPACE
class QPushButton;
class QRadioButton;
QT_END_NAMESPACE

class CompositionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CompositionWidget(QWidget *parent = nullptr);
    ~CompositionWidget();

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
    explicit CompositionRenderer(QWidget *parent = nullptr);
    ~CompositionRenderer();

    void paint(QPainter *) override;

    void setCirclePos(const QPointF &pos);

    QSize sizeHint() const override { return QSize(500, 400); }

    bool animationEnabled() const { return m_animation_enabled; }
    int circleColor() const { return m_circle_hue; }
    int circleAlpha() const { return m_circle_alpha; }

protected:
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void timerEvent(QTimerEvent *) override;

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

#if QT_CONFIG(opengl)
    std::unique_ptr<QFboPaintDevice> m_fbo;
    int m_pbuffer_size; // width==height==size of pbuffer
    uint m_base_tex;
    QSize m_previous_size;
    QOpenGLTextureBlitter m_blitter;
#endif
};

#endif // COMPOSITION_H
