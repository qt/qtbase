/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "pathdeform.h"

#include <QGuiApplication>
#include <QScreen>
#include <QtDebug>
#include <QMouseEvent>
#include <QTimerEvent>
#include <QLayout>
#include <QLineEdit>
#include <QPainter>
#include <QSlider>
#include <QLabel>
#include <QDesktopWidget>
#include <qmath.h>

PathDeformControls::PathDeformControls(QWidget *parent,
                                       PathDeformRenderer* renderer, bool smallScreen)
      : QWidget(parent)
{
    m_renderer = renderer;

    if (smallScreen)
        layoutForSmallScreen();
    else
        layoutForDesktop();
}

void PathDeformControls::layoutForDesktop()
{
    QGroupBox* mainGroup = new QGroupBox(this);
    mainGroup->setTitle(tr("Controls"));

    QGroupBox *radiusGroup = new QGroupBox(mainGroup);
    radiusGroup->setTitle(tr("Lens Radius"));
    QSlider *radiusSlider = new QSlider(Qt::Horizontal, radiusGroup);
    radiusSlider->setRange(15, 150);
    radiusSlider->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QGroupBox *deformGroup = new QGroupBox(mainGroup);
    deformGroup->setTitle(tr("Deformation"));
    QSlider *deformSlider = new QSlider(Qt::Horizontal, deformGroup);
    deformSlider->setRange(-100, 100);
    deformSlider->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QGroupBox *fontSizeGroup = new QGroupBox(mainGroup);
    fontSizeGroup->setTitle(tr("Font Size"));
    QSlider *fontSizeSlider = new QSlider(Qt::Horizontal, fontSizeGroup);
    fontSizeSlider->setRange(16, 200);
    fontSizeSlider->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QGroupBox *textGroup = new QGroupBox(mainGroup);
    textGroup->setTitle(tr("Text"));
    QLineEdit *textInput = new QLineEdit(textGroup);

    QPushButton *animateButton = new QPushButton(mainGroup);
    animateButton->setText(tr("Animated"));
    animateButton->setCheckable(true);

    QPushButton *showSourceButton = new QPushButton(mainGroup);
    showSourceButton->setText(tr("Show Source"));

#if QT_CONFIG(opengl)
    QPushButton *enableOpenGLButton = new QPushButton(mainGroup);
    enableOpenGLButton->setText(tr("Use OpenGL"));
    enableOpenGLButton->setCheckable(true);
    enableOpenGLButton->setChecked(m_renderer->usesOpenGL());
#endif

    QPushButton *whatsThisButton = new QPushButton(mainGroup);
    whatsThisButton->setText(tr("What's This?"));
    whatsThisButton->setCheckable(true);


    mainGroup->setFixedWidth(180);

    QVBoxLayout *mainGroupLayout = new QVBoxLayout(mainGroup);
    mainGroupLayout->addWidget(radiusGroup);
    mainGroupLayout->addWidget(deformGroup);
    mainGroupLayout->addWidget(fontSizeGroup);
    mainGroupLayout->addWidget(textGroup);
    mainGroupLayout->addWidget(animateButton);
    mainGroupLayout->addStretch(1);
#if QT_CONFIG(opengl)
    mainGroupLayout->addWidget(enableOpenGLButton);
#endif
    mainGroupLayout->addWidget(showSourceButton);
    mainGroupLayout->addWidget(whatsThisButton);

    QVBoxLayout *radiusGroupLayout = new QVBoxLayout(radiusGroup);
    radiusGroupLayout->addWidget(radiusSlider);

    QVBoxLayout *deformGroupLayout = new QVBoxLayout(deformGroup);
    deformGroupLayout->addWidget(deformSlider);

    QVBoxLayout *fontSizeGroupLayout = new QVBoxLayout(fontSizeGroup);
    fontSizeGroupLayout->addWidget(fontSizeSlider);

    QVBoxLayout *textGroupLayout = new QVBoxLayout(textGroup);
    textGroupLayout->addWidget(textInput);

    QVBoxLayout * mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainGroup);
    mainLayout->setContentsMargins(QMargins());

    connect(radiusSlider, &QAbstractSlider::valueChanged, m_renderer, &PathDeformRenderer::setRadius);
    connect(deformSlider, &QAbstractSlider::valueChanged, m_renderer, &PathDeformRenderer::setIntensity);
    connect(fontSizeSlider, &QAbstractSlider::valueChanged, m_renderer, &PathDeformRenderer::setFontSize);
    connect(animateButton, &QAbstractButton::clicked, m_renderer, &PathDeformRenderer::setAnimated);
#if QT_CONFIG(opengl)
    connect(enableOpenGLButton, &QAbstractButton::clicked, m_renderer, &ArthurFrame::enableOpenGL);
#endif

    connect(textInput, &QLineEdit::textChanged, m_renderer, &PathDeformRenderer::setText);
    connect(m_renderer, &ArthurFrame::descriptionEnabledChanged,
            whatsThisButton, &QAbstractButton::setChecked);
    connect(whatsThisButton, &QAbstractButton::clicked, m_renderer, &ArthurFrame::setDescriptionEnabled);
    connect(showSourceButton, &QAbstractButton::clicked, m_renderer, &ArthurFrame::showSource);

    animateButton->animateClick();
    deformSlider->setValue(80);
    fontSizeSlider->setValue(120);
    radiusSlider->setValue(100);
    textInput->setText(tr("Qt"));
}

void PathDeformControls::layoutForSmallScreen()
{
    QGroupBox* mainGroup = new QGroupBox(this);
    mainGroup->setTitle(tr("Controls"));

    QLabel *radiusLabel = new QLabel(mainGroup);
    radiusLabel->setText(tr("Lens Radius:"));
    QSlider *radiusSlider = new QSlider(Qt::Horizontal, mainGroup);
    radiusSlider->setRange(15, 150);
    radiusSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QLabel *deformLabel = new QLabel(mainGroup);
    deformLabel->setText(tr("Deformation:"));
    QSlider *deformSlider = new QSlider(Qt::Horizontal, mainGroup);
    deformSlider->setRange(-100, 100);
    deformSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QLabel *fontSizeLabel = new QLabel(mainGroup);
    fontSizeLabel->setText(tr("Font Size:"));
    QSlider *fontSizeSlider = new QSlider(Qt::Horizontal, mainGroup);
    fontSizeSlider->setRange(16, 200);
    fontSizeSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QPushButton *animateButton = new QPushButton(tr("Animated"), mainGroup);
    animateButton->setCheckable(true);

#if QT_CONFIG(opengl)
    QPushButton *enableOpenGLButton = new QPushButton(mainGroup);
    enableOpenGLButton->setText(tr("Use OpenGL"));
    enableOpenGLButton->setCheckable(mainGroup);
    enableOpenGLButton->setChecked(m_renderer->usesOpenGL());
#endif

    QPushButton *quitButton = new QPushButton(tr("Quit"), mainGroup);
    QPushButton *okButton = new QPushButton(tr("OK"), mainGroup);


    QGridLayout *mainGroupLayout = new QGridLayout(mainGroup);
    mainGroupLayout->setContentsMargins(QMargins());
    mainGroupLayout->addWidget(radiusLabel, 0, 0, Qt::AlignRight);
    mainGroupLayout->addWidget(radiusSlider, 0, 1);
    mainGroupLayout->addWidget(deformLabel, 1, 0, Qt::AlignRight);
    mainGroupLayout->addWidget(deformSlider, 1, 1);
    mainGroupLayout->addWidget(fontSizeLabel, 2, 0, Qt::AlignRight);
    mainGroupLayout->addWidget(fontSizeSlider, 2, 1);
    mainGroupLayout->addWidget(animateButton, 3,0, 1,2);
#if QT_CONFIG(opengl)
    mainGroupLayout->addWidget(enableOpenGLButton, 4,0, 1,2);
#endif

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainGroup);
    mainLayout->addStretch(1);
    mainLayout->addWidget(okButton);
    mainLayout->addWidget(quitButton);

    connect(quitButton, &QAbstractButton::clicked, this, &PathDeformControls::quitPressed);
    connect(okButton, &QAbstractButton::clicked, this, &PathDeformControls::okPressed);
    connect(radiusSlider, &QAbstractSlider::valueChanged, m_renderer, &PathDeformRenderer::setRadius);
    connect(deformSlider, &QAbstractSlider::valueChanged, m_renderer, &PathDeformRenderer::setIntensity);
    connect(fontSizeSlider, &QAbstractSlider::valueChanged, m_renderer, &PathDeformRenderer::setFontSize);
    connect(animateButton, &QAbstractButton::clicked, m_renderer, &PathDeformRenderer::setAnimated);
#if QT_CONFIG(opengl)
    connect(enableOpenGLButton, &QAbstractButton::clicked, m_renderer, &ArthurFrame::enableOpenGL);
#endif


    animateButton->animateClick();
    deformSlider->setValue(80);
    fontSizeSlider->setValue(120);

    QRect screen_size = QGuiApplication::primaryScreen()->geometry();
    radiusSlider->setValue(qMin(screen_size.width(), screen_size.height())/5);

    m_renderer->setText(tr("Qt"));
}

PathDeformWidget::PathDeformWidget(QWidget *parent, bool smallScreen)
    : QWidget(parent)
{
    setWindowTitle(tr("Vector Deformation"));

    m_renderer = new PathDeformRenderer(this, smallScreen);
    m_renderer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Layouts
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(m_renderer);

    m_controls = new PathDeformControls(nullptr, m_renderer, smallScreen);
    m_controls->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);

    if (!smallScreen)
        mainLayout->addWidget(m_controls);

    m_renderer->loadSourceFile(":res/deform/pathdeform.cpp");
    m_renderer->loadDescription(":res/deform/pathdeform.html");
    m_renderer->setDescriptionEnabled(false);

    connect(m_renderer, &PathDeformRenderer::clicked,
            this, &PathDeformWidget::showControls);
    connect(m_controls, &PathDeformControls::okPressed,
            this, &PathDeformWidget::hideControls);
    connect(m_controls, &PathDeformControls::quitPressed,
            qApp, &QCoreApplication::quit);
}


void PathDeformWidget::showControls()
{
    m_controls->showFullScreen();
}

void PathDeformWidget::hideControls()
{
    m_controls->hide();
}

void PathDeformWidget::setStyle(QStyle *style)
{
    QWidget::setStyle(style);
    if (!m_controls)
        return;

    m_controls->setStyle(style);

    const QList<QWidget *> widgets = m_controls->findChildren<QWidget *>();
    for (QWidget *w : widgets)
        w->setStyle(style);
}

static inline QRect circle_bounds(const QPointF &center, qreal radius, qreal compensation)
{
    return QRect(qRound(center.x() - radius - compensation),
                 qRound(center.y() - radius - compensation),
                 qRound((radius + compensation) * 2),
                 qRound((radius + compensation) * 2));

}

const int LENS_EXTENT = 10;

PathDeformRenderer::PathDeformRenderer(QWidget *widget, bool smallScreen)
    : ArthurFrame(widget)
{
    m_radius = 100;
    m_pos = QPointF(m_radius, m_radius);
    m_direction = QPointF(1, 1);
    m_fontSize = 24;
    m_animated = true;
    m_repaintTimer.start(25, this);
    m_repaintTracker.start();
    m_intensity = 100;
    m_smallScreen = smallScreen;

//     m_fpsTimer.start(1000, this);
//     m_fpsCounter = 0;

    generateLensPixmap();
}

void PathDeformRenderer::setText(const QString &text)
{
    m_text = text;

    QFont f("times new roman,utopia");
    f.setStyleStrategy(QFont::ForceOutline);
    f.setPointSize(m_fontSize);
    f.setStyleHint(QFont::Times);

    QFontMetrics fm(f);

    m_paths.clear();
    m_pathBounds = QRect();

    QPointF advance(0, 0);

    bool do_quick = true;
    for (int i=0; i<text.size(); ++i) {
        if (text.at(i).unicode() >= 0x4ff && text.at(i).unicode() <= 0x1e00) {
            do_quick = false;
            break;
        }
    }

    if (do_quick) {
        for (int i=0; i<text.size(); ++i) {
            QPainterPath path;
            path.addText(advance, f, text.mid(i, 1));
            m_pathBounds |= path.boundingRect();
            m_paths << path;
            advance += QPointF(fm.horizontalAdvance(text.mid(i, 1)), 0);
        }
    } else {
        QPainterPath path;
        path.addText(advance, f, text);
        m_pathBounds |= path.boundingRect();
        m_paths << path;
    }

    for (int i=0; i<m_paths.size(); ++i)
        m_paths[i] = m_paths[i] * QTransform(1, 0, 0, 1, -m_pathBounds.x(), -m_pathBounds.y());

    update();
}


void PathDeformRenderer::generateLensPixmap()
{
    qreal rad = m_radius + LENS_EXTENT;

    QRect bounds = circle_bounds(QPointF(), rad, 0);

    QPainter painter;

    if (preferImage()) {
        m_lens_image = QImage(bounds.size(), QImage::Format_ARGB32_Premultiplied);
        m_lens_image.fill(0);
        painter.begin(&m_lens_image);
    } else {
        m_lens_pixmap = QPixmap(bounds.size());
        m_lens_pixmap.fill(Qt::transparent);
        painter.begin(&m_lens_pixmap);
    }

    QRadialGradient gr(rad, rad, rad, 3 * rad / 5, 3 * rad / 5);
    gr.setColorAt(0.0, QColor(255, 255, 255, 191));
    gr.setColorAt(0.2, QColor(255, 255, 127, 191));
    gr.setColorAt(0.9, QColor(150, 150, 200, 63));
    gr.setColorAt(0.95, QColor(0, 0, 0, 127));
    gr.setColorAt(1, QColor(0, 0, 0, 0));
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(gr);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, bounds.width(), bounds.height());
}


void PathDeformRenderer::setAnimated(bool animated)
{
    m_animated = animated;

    if (m_animated) {
//         m_fpsTimer.start(1000, this);
//         m_fpsCounter = 0;
        m_repaintTimer.start(25, this);
        m_repaintTracker.start();
    } else {
//         m_fpsTimer.stop();
        m_repaintTimer.stop();
    }
}

void PathDeformRenderer::timerEvent(QTimerEvent *e)
{

    if (e->timerId() == m_repaintTimer.timerId()) {

        if (QLineF(QPointF(0,0), m_direction).length() > 1)
            m_direction *= 0.995;
        qreal time = m_repaintTracker.restart();

        QRect rectBefore = circle_bounds(m_pos, m_radius, m_fontSize);

        qreal dx = m_direction.x();
        qreal dy = m_direction.y();
        if (time > 0) {
            dx = dx * time * .1;
            dy = dy * time * .1;
        }

        m_pos += QPointF(dx, dy);

        if (m_pos.x() - m_radius < 0) {
            m_direction.setX(-m_direction.x());
            m_pos.setX(m_radius);
        } else if (m_pos.x() + m_radius > width()) {
            m_direction.setX(-m_direction.x());
            m_pos.setX(width() - m_radius);
        }

        if (m_pos.y() - m_radius < 0) {
            m_direction.setY(-m_direction.y());
            m_pos.setY(m_radius);
        } else if (m_pos.y() + m_radius > height()) {
            m_direction.setY(-m_direction.y());
            m_pos.setY(height() - m_radius);
        }

#if QT_CONFIG(opengl)
        if (usesOpenGL()) {
            update();
        } else
#endif
        {
            QRect rectAfter = circle_bounds(m_pos, m_radius, m_fontSize);
            update(rectAfter | rectBefore);
        }
    }
//     else if (e->timerId() == m_fpsTimer.timerId()) {
//         printf("fps: %d\n", m_fpsCounter);
//         emit frameRate(m_fpsCounter);
//         m_fpsCounter = 0;

//     }
}

void PathDeformRenderer::mousePressEvent(QMouseEvent *e)
{
    if (m_show_doc) {
        setDescriptionEnabled(false);
        return;
    }
    setDescriptionEnabled(false);

    m_repaintTimer.stop();
    m_offset = QPointF();
    if (QLineF(m_pos, e->pos()).length() <= m_radius)
        m_offset = m_pos - e->pos();

    m_mousePress = e->pos();

    // If we're not running in small screen mode, always assume we're dragging
    m_mouseDrag = !m_smallScreen;

    mouseMoveEvent(e);
}

void PathDeformRenderer::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->buttons() == Qt::NoButton && m_animated) {
        m_repaintTimer.start(10, this);
        m_repaintTracker.start();
    }

    if (!m_mouseDrag && m_smallScreen)
        emit clicked();
}

void PathDeformRenderer::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_mouseDrag && (QLineF(m_mousePress, e->pos()).length() > 25.0) )
        m_mouseDrag = true;

    if (m_mouseDrag) {
        QRect rectBefore = circle_bounds(m_pos, m_radius, m_fontSize);
        if (e->type() == QEvent::MouseMove) {
            QLineF line(m_pos, e->pos() + m_offset);
            line.setLength(line.length() * .1);
            QPointF dir(line.dx(), line.dy());
            m_direction = (m_direction + dir) / 2;
        }
        m_pos = e->pos() + m_offset;
#if QT_CONFIG(opengl)
        if (usesOpenGL()) {
            update();
        } else
#endif
        {
            QRect rectAfter = circle_bounds(m_pos, m_radius, m_fontSize);
            update(rectBefore | rectAfter);
        }
    }
}

QPainterPath PathDeformRenderer::lensDeform(const QPainterPath &source, const QPointF &offset)
{
    QPainterPath path;
    path.addPath(source);

    qreal flip = m_intensity / qreal(100);

    for (int i=0; i<path.elementCount(); ++i) {
        const QPainterPath::Element &e = path.elementAt(i);

        qreal x = e.x + offset.x();
        qreal y = e.y + offset.y();

        qreal dx = x - m_pos.x();
        qreal dy = y - m_pos.y();
        qreal len = m_radius - qSqrt(dx * dx + dy * dy);

        if (len > 0) {
            path.setElementPositionAt(i,
                                      x + flip * dx * len / m_radius,
                                      y + flip * dy * len / m_radius);
        } else {
            path.setElementPositionAt(i, x, y);
        }

    }

    return path;
}

void PathDeformRenderer::paint(QPainter *painter)
{
    int pad_x = 5;
    int pad_y = 5;

    int skip_x = qRound(m_pathBounds.width() + pad_x + m_fontSize/2);
    int skip_y = qRound(m_pathBounds.height() + pad_y);

    painter->setPen(Qt::NoPen);
    painter->setBrush(Qt::black);

    QRectF clip(painter->clipPath().boundingRect());

    int overlap = pad_x / 2;

    for (int start_y=0; start_y < height(); start_y += skip_y) {

        if (start_y > clip.bottom())
            break;

        int start_x = -overlap;
        for (; start_x < width(); start_x += skip_x) {

            if (start_y + skip_y >= clip.top() &&
                start_x + skip_x >= clip.left() &&
                start_x <= clip.right()) {
                for (int i=0; i<m_paths.size(); ++i) {
                    QPainterPath path = lensDeform(m_paths[i], QPointF(start_x, start_y));
                    painter->drawPath(path);
                }
            }
        }
        overlap = skip_x - (start_x - width());

    }

    if (preferImage()) {
        painter->drawImage(m_pos - QPointF(m_radius + LENS_EXTENT, m_radius + LENS_EXTENT),
                           m_lens_image);
    } else {
        painter->drawPixmap(m_pos - QPointF(m_radius + LENS_EXTENT, m_radius + LENS_EXTENT),
                            m_lens_pixmap);
    }
}

void PathDeformRenderer::setRadius(int radius)
{
    qreal max = qMax(m_radius, (qreal)radius);
    m_radius = radius;
    generateLensPixmap();
    if (!m_animated || m_radius < max) {
#if QT_CONFIG(opengl)
        if (usesOpenGL()){
            update();
            return;
        }
#endif
        update(circle_bounds(m_pos, max, m_fontSize));
    }
}

void PathDeformRenderer::setIntensity(int intensity)
{
    m_intensity = intensity;
    if (!m_animated) {
#if QT_CONFIG(opengl)
        if (usesOpenGL()) {
            update();
            return;
        }
#endif
        update(circle_bounds(m_pos, m_radius, m_fontSize));
    }
}
