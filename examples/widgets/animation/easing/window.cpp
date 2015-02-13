/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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

#include "window.h"

Window::Window(QWidget *parent)
    : QWidget(parent),
    m_iconSize(64, 64)
{
    m_ui.setupUi(this);
    QButtonGroup *buttonGroup = findChild<QButtonGroup *>();     // ### workaround for uic in 4.4
    m_ui.easingCurvePicker->setIconSize(m_iconSize);
    m_ui.easingCurvePicker->setMinimumHeight(m_iconSize.height() + 50);
    buttonGroup->setId(m_ui.lineRadio, 0);
    buttonGroup->setId(m_ui.circleRadio, 1);

    QEasingCurve dummy;
    m_ui.periodSpinBox->setValue(dummy.period());
    m_ui.amplitudeSpinBox->setValue(dummy.amplitude());
    m_ui.overshootSpinBox->setValue(dummy.overshoot());

    connect(m_ui.easingCurvePicker, SIGNAL(currentRowChanged(int)), this, SLOT(curveChanged(int)));
    connect(buttonGroup, SIGNAL(buttonClicked(int)), this, SLOT(pathChanged(int)));
    connect(m_ui.periodSpinBox, SIGNAL(valueChanged(double)), this, SLOT(periodChanged(double)));
    connect(m_ui.amplitudeSpinBox, SIGNAL(valueChanged(double)), this, SLOT(amplitudeChanged(double)));
    connect(m_ui.overshootSpinBox, SIGNAL(valueChanged(double)), this, SLOT(overshootChanged(double)));
    createCurveIcons();

    QPixmap pix(QLatin1String(":/images/qt-logo.png"));
    m_item = new PixmapItem(pix);
    m_scene.addItem(m_item);
    m_ui.graphicsView->setScene(&m_scene);

    m_anim = new Animation(m_item, "pos");
    m_anim->setEasingCurve(QEasingCurve::OutBounce);
    m_ui.easingCurvePicker->setCurrentRow(int(QEasingCurve::OutBounce));

    startAnimation();
}

QEasingCurve createEasingCurve(QEasingCurve::Type curveType)
{
    QEasingCurve curve(curveType);

    if (curveType == QEasingCurve::BezierSpline) {
        curve.addCubicBezierSegment(QPointF(0.4, 0.1), QPointF(0.6, 0.9), QPointF(1.0, 1.0));

    } else if (curveType == QEasingCurve::TCBSpline) {
        curve.addTCBSegment(QPointF(0.0, 0.0), 0, 0, 0);
        curve.addTCBSegment(QPointF(0.3, 0.4), 0.2, 1, -0.2);
        curve.addTCBSegment(QPointF(0.7, 0.6), -0.2, 1, 0.2);
        curve.addTCBSegment(QPointF(1.0, 1.0), 0, 0, 0);
    }

    return curve;
}

void Window::createCurveIcons()
{
    QPixmap pix(m_iconSize);
    QPainter painter(&pix);
    QLinearGradient gradient(0,0, 0, m_iconSize.height());
    gradient.setColorAt(0.0, QColor(240, 240, 240));
    gradient.setColorAt(1.0, QColor(224, 224, 224));
    QBrush brush(gradient);
    const QMetaObject &mo = QEasingCurve::staticMetaObject;
    QMetaEnum metaEnum = mo.enumerator(mo.indexOfEnumerator("Type"));
    // Skip QEasingCurve::Custom
    for (int i = 0; i < QEasingCurve::NCurveTypes - 1; ++i) {
        painter.fillRect(QRect(QPoint(0, 0), m_iconSize), brush);
        QEasingCurve curve = createEasingCurve((QEasingCurve::Type) i);
        painter.setPen(QColor(0, 0, 255, 64));
        qreal xAxis = m_iconSize.height()/1.5;
        qreal yAxis = m_iconSize.width()/3;
        painter.drawLine(0, xAxis, m_iconSize.width(),  xAxis);
        painter.drawLine(yAxis, 0, yAxis, m_iconSize.height());

        qreal curveScale = m_iconSize.height()/2;

        painter.setPen(Qt::NoPen);

        // start point
        painter.setBrush(Qt::red);
        QPoint start(yAxis, xAxis - curveScale * curve.valueForProgress(0));
        painter.drawRect(start.x() - 1, start.y() - 1, 3, 3);

        // end point
        painter.setBrush(Qt::blue);
        QPoint end(yAxis + curveScale, xAxis - curveScale * curve.valueForProgress(1));
        painter.drawRect(end.x() - 1, end.y() - 1, 3, 3);

        QPainterPath curvePath;
        curvePath.moveTo(start);
        for (qreal t = 0; t <= 1.0; t+=1.0/curveScale) {
            QPoint to;
            to.setX(yAxis + curveScale * t);
            to.setY(xAxis - curveScale * curve.valueForProgress(t));
            curvePath.lineTo(to);
        }
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.strokePath(curvePath, QColor(32, 32, 32));
        painter.setRenderHint(QPainter::Antialiasing, false);
        QListWidgetItem *item = new QListWidgetItem;
        item->setIcon(QIcon(pix));
        item->setText(metaEnum.key(i));
        m_ui.easingCurvePicker->addItem(item);
    }
}

void Window::startAnimation()
{
    m_anim->setStartValue(QPointF(0, 0));
    m_anim->setEndValue(QPointF(100, 100));
    m_anim->setDuration(2000);
    m_anim->setLoopCount(-1); // forever
    m_anim->start();
}

void Window::curveChanged(int row)
{
    QEasingCurve::Type curveType = (QEasingCurve::Type)row;
    m_anim->setEasingCurve(createEasingCurve(curveType));
    m_anim->setCurrentTime(0);

    bool isElastic = curveType >= QEasingCurve::InElastic && curveType <= QEasingCurve::OutInElastic;
    bool isBounce = curveType >= QEasingCurve::InBounce && curveType <= QEasingCurve::OutInBounce;
    m_ui.periodSpinBox->setEnabled(isElastic);
    m_ui.amplitudeSpinBox->setEnabled(isElastic || isBounce);
    m_ui.overshootSpinBox->setEnabled(curveType >= QEasingCurve::InBack && curveType <= QEasingCurve::OutInBack);
}

void Window::pathChanged(int index)
{
    m_anim->setPathType((Animation::PathType)index);
}

void Window::periodChanged(double value)
{
    QEasingCurve curve = m_anim->easingCurve();
    curve.setPeriod(value);
    m_anim->setEasingCurve(curve);
}

void Window::amplitudeChanged(double value)
{
    QEasingCurve curve = m_anim->easingCurve();
    curve.setAmplitude(value);
    m_anim->setEasingCurve(curve);
}

void Window::overshootChanged(double value)
{
    QEasingCurve curve = m_anim->easingCurve();
    curve.setOvershoot(value);
    m_anim->setEasingCurve(curve);
}

