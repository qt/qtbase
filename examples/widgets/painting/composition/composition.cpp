// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "composition.h"
#include <QBoxLayout>
#include <QRadioButton>
#include <QTimer>
#include <QDateTime>
#include <QSlider>
#include <QMouseEvent>
#include <qmath.h>

using namespace std::chrono_literals;

const auto animationInterval = 15ms; // update every 16 ms = ~60FPS

CompositionWidget::CompositionWidget(QWidget *parent)
    : QWidget(parent)
{
    CompositionRenderer *view = new CompositionRenderer(this);

    QGroupBox *mainGroup = new QGroupBox(parent);
    mainGroup->setTitle(tr("Composition Modes"));

    QGroupBox *modesGroup = new QGroupBox(mainGroup);
    modesGroup->setTitle(tr("Mode"));

    rbClear = new QRadioButton(tr("Clear"), modesGroup);
    connect(rbClear, &QAbstractButton::clicked, view, &CompositionRenderer::setClearMode);
    rbSource = new QRadioButton(tr("Source"), modesGroup);
    connect(rbSource, &QAbstractButton::clicked, view, &CompositionRenderer::setSourceMode);
    rbDest = new QRadioButton(tr("Destination"), modesGroup);
    connect(rbDest, &QAbstractButton::clicked, view, &CompositionRenderer::setDestMode);
    rbSourceOver = new QRadioButton(tr("Source Over"), modesGroup);
    connect(rbSourceOver, &QAbstractButton::clicked, view, &CompositionRenderer::setSourceOverMode);
    rbDestOver = new QRadioButton(tr("Destination Over"), modesGroup);
    connect(rbDestOver, &QAbstractButton::clicked, view, &CompositionRenderer::setDestOverMode);
    rbSourceIn = new QRadioButton(tr("Source In"), modesGroup);
    connect(rbSourceIn, &QAbstractButton::clicked, view, &CompositionRenderer::setSourceInMode);
    rbDestIn = new QRadioButton(tr("Dest In"), modesGroup);
    connect(rbDestIn, &QAbstractButton::clicked, view, &CompositionRenderer::setDestInMode);
    rbSourceOut = new QRadioButton(tr("Source Out"), modesGroup);
    connect(rbSourceOut, &QAbstractButton::clicked, view, &CompositionRenderer::setSourceOutMode);
    rbDestOut = new QRadioButton(tr("Dest Out"), modesGroup);
    connect(rbDestOut, &QAbstractButton::clicked, view, &CompositionRenderer::setDestOutMode);
    rbSourceAtop = new QRadioButton(tr("Source Atop"), modesGroup);
    connect(rbSourceAtop, &QAbstractButton::clicked, view, &CompositionRenderer::setSourceAtopMode);
    rbDestAtop = new QRadioButton(tr("Dest Atop"), modesGroup);
    connect(rbDestAtop, &QAbstractButton::clicked, view, &CompositionRenderer::setDestAtopMode);
    rbXor = new QRadioButton(tr("Xor"), modesGroup);
    connect(rbXor, &QAbstractButton::clicked, view, &CompositionRenderer::setXorMode);

    rbPlus = new QRadioButton(tr("Plus"), modesGroup);
    connect(rbPlus, &QAbstractButton::clicked, view, &CompositionRenderer::setPlusMode);
    rbMultiply = new QRadioButton(tr("Multiply"), modesGroup);
    connect(rbMultiply, &QAbstractButton::clicked, view, &CompositionRenderer::setMultiplyMode);
    rbScreen = new QRadioButton(tr("Screen"), modesGroup);
    connect(rbScreen, &QAbstractButton::clicked, view, &CompositionRenderer::setScreenMode);
    rbOverlay = new QRadioButton(tr("Overlay"), modesGroup);
    connect(rbOverlay, &QAbstractButton::clicked, view, &CompositionRenderer::setOverlayMode);
    rbDarken = new QRadioButton(tr("Darken"), modesGroup);
    connect(rbDarken, &QAbstractButton::clicked, view, &CompositionRenderer::setDarkenMode);
    rbLighten = new QRadioButton(tr("Lighten"), modesGroup);
    connect(rbLighten, &QAbstractButton::clicked, view, &CompositionRenderer::setLightenMode);
    rbColorDodge = new QRadioButton(tr("Color Dodge"), modesGroup);
    connect(rbColorDodge, &QAbstractButton::clicked, view, &CompositionRenderer::setColorDodgeMode);
    rbColorBurn = new QRadioButton(tr("Color Burn"), modesGroup);
    connect(rbColorBurn, &QAbstractButton::clicked, view, &CompositionRenderer::setColorBurnMode);
    rbHardLight = new QRadioButton(tr("Hard Light"), modesGroup);
    connect(rbHardLight, &QAbstractButton::clicked, view, &CompositionRenderer::setHardLightMode);
    rbSoftLight = new QRadioButton(tr("Soft Light"), modesGroup);
    connect(rbSoftLight, &QAbstractButton::clicked, view, &CompositionRenderer::setSoftLightMode);
    rbDifference = new QRadioButton(tr("Difference"), modesGroup);
    connect(rbDifference, &QAbstractButton::clicked, view, &CompositionRenderer::setDifferenceMode);
    rbExclusion = new QRadioButton(tr("Exclusion"), modesGroup);
    connect(rbExclusion, &QAbstractButton::clicked, view, &CompositionRenderer::setExclusionMode);

    QGroupBox *circleColorGroup = new QGroupBox(mainGroup);
    circleColorGroup->setTitle(tr("Circle color"));
    QSlider *circleColorSlider = new QSlider(Qt::Horizontal, circleColorGroup);
    circleColorSlider->setRange(0, 359);
    circleColorSlider->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    connect(circleColorSlider, &QAbstractSlider::valueChanged, view, &CompositionRenderer::setCircleColor);

    QGroupBox *circleAlphaGroup = new QGroupBox(mainGroup);
    circleAlphaGroup->setTitle(tr("Circle alpha"));
    QSlider *circleAlphaSlider = new QSlider(Qt::Horizontal, circleAlphaGroup);
    circleAlphaSlider->setRange(0, 255);
    circleAlphaSlider->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    connect(circleAlphaSlider, &QAbstractSlider::valueChanged, view, &CompositionRenderer::setCircleAlpha);

    QPushButton *showSourceButton = new QPushButton(mainGroup);
    showSourceButton->setText(tr("Show Source"));
    QPushButton *whatsThisButton = new QPushButton(mainGroup);
    whatsThisButton->setText(tr("What's This?"));
    whatsThisButton->setCheckable(true);

    QPushButton *animateButton = new QPushButton(mainGroup);
    animateButton->setText(tr("Animated"));
    animateButton->setCheckable(true);
    animateButton->setChecked(true);

    QHBoxLayout *viewLayout = new QHBoxLayout(this);
    viewLayout->addWidget(view);
    viewLayout->addWidget(mainGroup);

    QVBoxLayout *mainGroupLayout = new QVBoxLayout(mainGroup);
    mainGroupLayout->addWidget(circleColorGroup);
    mainGroupLayout->addWidget(circleAlphaGroup);
    mainGroupLayout->addWidget(modesGroup);
    mainGroupLayout->addStretch();
    mainGroupLayout->addWidget(animateButton);
    mainGroupLayout->addWidget(whatsThisButton);
    mainGroupLayout->addWidget(showSourceButton);

    QGridLayout *modesLayout = new QGridLayout(modesGroup);
    modesLayout->addWidget(rbClear, 0, 0);
    modesLayout->addWidget(rbSource, 1, 0);
    modesLayout->addWidget(rbDest, 2, 0);
    modesLayout->addWidget(rbSourceOver, 3, 0);
    modesLayout->addWidget(rbDestOver, 4, 0);
    modesLayout->addWidget(rbSourceIn, 5, 0);
    modesLayout->addWidget(rbDestIn, 6, 0);
    modesLayout->addWidget(rbSourceOut, 7, 0);
    modesLayout->addWidget(rbDestOut, 8, 0);
    modesLayout->addWidget(rbSourceAtop, 9, 0);
    modesLayout->addWidget(rbDestAtop, 10, 0);
    modesLayout->addWidget(rbXor, 11, 0);

    modesLayout->addWidget(rbPlus, 0, 1);
    modesLayout->addWidget(rbMultiply, 1, 1);
    modesLayout->addWidget(rbScreen, 2, 1);
    modesLayout->addWidget(rbOverlay, 3, 1);
    modesLayout->addWidget(rbDarken, 4, 1);
    modesLayout->addWidget(rbLighten, 5, 1);
    modesLayout->addWidget(rbColorDodge, 6, 1);
    modesLayout->addWidget(rbColorBurn, 7, 1);
    modesLayout->addWidget(rbHardLight, 8, 1);
    modesLayout->addWidget(rbSoftLight, 9, 1);
    modesLayout->addWidget(rbDifference, 10, 1);
    modesLayout->addWidget(rbExclusion, 11, 1);


    QVBoxLayout *circleColorLayout = new QVBoxLayout(circleColorGroup);
    circleColorLayout->addWidget(circleColorSlider);

    QVBoxLayout *circleAlphaLayout = new QVBoxLayout(circleAlphaGroup);
    circleAlphaLayout->addWidget(circleAlphaSlider);

    view->loadDescription(":res/composition/composition.html");
    view->loadSourceFile(":res/composition/composition.cpp");

    connect(whatsThisButton, &QAbstractButton::clicked, view, &ArthurFrame::setDescriptionEnabled);
    connect(view, &ArthurFrame::descriptionEnabledChanged, whatsThisButton, &QAbstractButton::setChecked);
    connect(showSourceButton, &QAbstractButton::clicked, view, &ArthurFrame::showSource);
    connect(animateButton, &QAbstractButton::toggled, view, &CompositionRenderer::setAnimationEnabled);

    circleColorSlider->setValue(270);
    circleAlphaSlider->setValue(200);
    rbSourceOut->animateClick();

    setWindowTitle(tr("Composition Modes"));
}

CompositionWidget::~CompositionWidget()
{
}


void CompositionWidget::nextMode()
{
    /*
      if (!m_animation_enabled)
      return;
      if (rbClear->isChecked()) rbSource->animateClick();
      if (rbSource->isChecked()) rbDest->animateClick();
      if (rbDest->isChecked()) rbSourceOver->animateClick();
      if (rbSourceOver->isChecked()) rbDestOver->animateClick();
      if (rbDestOver->isChecked()) rbSourceIn->animateClick();
      if (rbSourceIn->isChecked()) rbDestIn->animateClick();
      if (rbDestIn->isChecked()) rbSourceOut->animateClick();
      if (rbSourceOut->isChecked()) rbDestOut->animateClick();
      if (rbDestOut->isChecked()) rbSourceAtop->animateClick();
      if (rbSourceAtop->isChecked()) rbDestAtop->animateClick();
      if (rbDestAtop->isChecked()) rbXor->animateClick();
      if (rbXor->isChecked()) rbClear->animateClick();
    */
}

CompositionRenderer::CompositionRenderer(QWidget *parent)
    : ArthurFrame(parent)
{
    m_animation_enabled = true;
    m_animationTimer.start(animationInterval, this);
    m_image = QImage(":res/composition/flower.jpg");
    m_image.setAlphaChannel(QImage(":res/composition/flower_alpha.jpg"));
    m_circle_alpha = 127;
    m_circle_hue = 255;
    m_current_object = NoObject;
    m_composition_mode = QPainter::CompositionMode_SourceOut;

    m_circle_pos = QPoint(200, 100);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

CompositionRenderer::~CompositionRenderer()
{
}

QRectF rectangle_around(const QPointF &p, const QSizeF &size = QSize(250, 200))
{
    QRectF rect(p, size);
    rect.translate(-size.width()/2, -size.height()/2);
    return rect;
}

void CompositionRenderer::setAnimationEnabled(bool enabled)
{
    if (m_animation_enabled == enabled)
        return;
    m_animation_enabled = enabled;
    if (enabled) {
        Q_ASSERT(!m_animationTimer.isActive());
        m_animationTimer.start(animationInterval, this);
    } else {
        m_animationTimer.stop();
    }
}

void CompositionRenderer::updateCirclePos()
{
    if (m_current_object != NoObject)
        return;
    QDateTime dt = QDateTime::currentDateTime();
    qreal t = dt.toMSecsSinceEpoch() / 1000.0;

    qreal x = width() / qreal(2) + (qCos(t*8/11) + qSin(-t)) * width() / qreal(4);
    qreal y = height() / qreal(2) + (qSin(t*6/7) + qCos(t * qreal(1.5))) * height() / qreal(4);

    setCirclePos(QLineF(m_circle_pos, QPointF(x, y)).pointAt(0.02));
}

void CompositionRenderer::drawBase(QPainter &p)
{
    p.setPen(Qt::NoPen);

    QLinearGradient rect_gradient(0, 0, 0, height());
    rect_gradient.setColorAt(0, Qt::red);
    rect_gradient.setColorAt(.17, Qt::yellow);
    rect_gradient.setColorAt(.33, Qt::green);
    rect_gradient.setColorAt(.50, Qt::cyan);
    rect_gradient.setColorAt(.66, Qt::blue);
    rect_gradient.setColorAt(.81, Qt::magenta);
    rect_gradient.setColorAt(1, Qt::red);
    p.setBrush(rect_gradient);
    p.drawRect(width() / 2, 0, width() / 2, height());

    QLinearGradient alpha_gradient(0, 0, width(), 0);
    alpha_gradient.setColorAt(0, Qt::white);
    alpha_gradient.setColorAt(0.2, Qt::white);
    alpha_gradient.setColorAt(0.5, Qt::transparent);
    alpha_gradient.setColorAt(0.8, Qt::white);
    alpha_gradient.setColorAt(1, Qt::white);

    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p.setBrush(alpha_gradient);
    p.drawRect(0, 0, width(), height());

    p.setCompositionMode(QPainter::CompositionMode_DestinationOver);

    p.setPen(Qt::NoPen);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawImage(rect(), m_image);
}

void CompositionRenderer::drawSource(QPainter &p)
{
    p.setPen(Qt::NoPen);
    p.setRenderHint(QPainter::Antialiasing);
    p.setCompositionMode(m_composition_mode);

    QRectF circle_rect = rectangle_around(m_circle_pos);
    QColor color = QColor::fromHsvF(m_circle_hue / 360.0, 1, 1, m_circle_alpha / 255.0);
    QLinearGradient circle_gradient(circle_rect.topLeft(), circle_rect.bottomRight());
    circle_gradient.setColorAt(0, color.lighter());
    circle_gradient.setColorAt(0.5, color);
    circle_gradient.setColorAt(1, color.darker());
    p.setBrush(circle_gradient);

    p.drawEllipse(circle_rect);
}

void CompositionRenderer::paint(QPainter *painter)
{
    if (m_buffer.size() != size()) {
        m_buffer = QImage(size(), QImage::Format_ARGB32_Premultiplied);
        m_base_buffer = QImage(size(), QImage::Format_ARGB32_Premultiplied);

        m_base_buffer.fill(0);

        QPainter p(&m_base_buffer);

        drawBase(p);
    }

    memcpy(m_buffer.bits(), m_base_buffer.bits(), m_buffer.sizeInBytes());

    {
        QPainter p(&m_buffer);
        drawSource(p);
    }

    painter->drawImage(0, 0, m_buffer);
}

void CompositionRenderer::mousePressEvent(QMouseEvent *e)
{
    setDescriptionEnabled(false);

    QRectF circle = rectangle_around(m_circle_pos);

    if (circle.contains(e->pos())) {
        m_current_object = Circle;
        m_offset = circle.center() - e->pos();
    } else {
        m_current_object = NoObject;
    }
    if (m_animation_enabled) {
        m_animationTimer.stop();
    }
}

void CompositionRenderer::mouseMoveEvent(QMouseEvent *e)
{
    if (m_current_object == Circle)
        setCirclePos(e->pos() + m_offset);
}

void CompositionRenderer::mouseReleaseEvent(QMouseEvent *)
{
    m_current_object = NoObject;

    if (m_animation_enabled) {
        Q_ASSERT(!m_animationTimer.isActive());
        m_animationTimer.start(animationInterval, this);
    }
}

void CompositionRenderer::timerEvent(QTimerEvent *event)
{
    if (event->matches(m_animationTimer))
        updateCirclePos();
}

void CompositionRenderer::setCirclePos(const QPointF &pos)
{
    const QRect oldRect = rectangle_around(m_circle_pos).toAlignedRect();
    m_circle_pos = pos;
    const QRect newRect = rectangle_around(m_circle_pos).toAlignedRect();
    update(oldRect | newRect);
}

