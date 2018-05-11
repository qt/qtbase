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

#ifndef PATHSTROKE_H
#define PATHSTROKE_H

#include "arthurwidgets.h"

#include <QtWidgets>

class PathStrokeRenderer : public ArthurFrame
{
    Q_OBJECT
    Q_PROPERTY(bool animation READ animation WRITE setAnimation)
    Q_PROPERTY(qreal penWidth READ realPenWidth WRITE setRealPenWidth)
public:
    enum PathMode { CurveMode, LineMode };

    explicit PathStrokeRenderer(QWidget *parent, bool smallScreen = false);

    void paint(QPainter *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void timerEvent(QTimerEvent *e) override;
    bool event(QEvent *e) override;

    QSize sizeHint() const override { return QSize(500, 500); }

    bool animation() const { return m_timer.isActive(); }

    qreal realPenWidth() const { return m_penWidth; }
    void setRealPenWidth(qreal penWidth) { m_penWidth = penWidth; update(); }

signals:
    void clicked();

public slots:
    void setPenWidth(int penWidth) { m_penWidth = penWidth / 10.0; update(); }
    void setAnimation(bool animation);

    void setFlatCap() { m_capStyle = Qt::FlatCap; update(); }
    void setSquareCap() { m_capStyle = Qt::SquareCap; update(); }
    void setRoundCap() { m_capStyle = Qt::RoundCap; update(); }

    void setBevelJoin() { m_joinStyle = Qt::BevelJoin; update(); }
    void setMiterJoin() { m_joinStyle = Qt::MiterJoin; update(); }
    void setSvgMiterJoin() { m_joinStyle = Qt::SvgMiterJoin; update(); }
    void setRoundJoin() { m_joinStyle = Qt::RoundJoin; update(); }

    void setCurveMode() { m_pathMode = CurveMode; update(); }
    void setLineMode() { m_pathMode = LineMode; update(); }

    void setSolidLine() { m_penStyle = Qt::SolidLine; update(); }
    void setDashLine() { m_penStyle = Qt::DashLine; update(); }
    void setDotLine() { m_penStyle = Qt::DotLine; update(); }
    void setDashDotLine() { m_penStyle = Qt::DashDotLine; update(); }
    void setDashDotDotLine() { m_penStyle = Qt::DashDotDotLine; update(); }
    void setCustomDashLine() { m_penStyle = Qt::NoPen; update(); }

private:
    void initializePoints();
    void updatePoints();

    QBasicTimer m_timer;

    PathMode m_pathMode;

    bool m_wasAnimated;

    qreal m_penWidth;
    int m_pointCount;
    int m_pointSize;
    int m_activePoint;
    QVector<QPointF> m_points;
    QVector<QPointF> m_vectors;

    Qt::PenJoinStyle m_joinStyle;
    Qt::PenCapStyle m_capStyle;

    Qt::PenStyle m_penStyle;

    bool m_smallScreen;
    QPoint m_mousePress;
    bool m_mouseDrag;

    QHash<int, int> m_fingerPointMapping;
};

class PathStrokeControls : public QWidget
{
    Q_OBJECT

public:
    PathStrokeControls(QWidget* parent, PathStrokeRenderer* renderer, bool smallScreen);

signals:
    void okPressed();
    void quitPressed();

private:
    PathStrokeRenderer* m_renderer;

    QGroupBox *m_capGroup;
    QGroupBox *m_joinGroup;
    QGroupBox *m_styleGroup;
    QGroupBox *m_pathModeGroup;

    void createCommonControls(QWidget* parent);
    void layoutForDesktop();
    void layoutForSmallScreens();

private slots:
    void emitQuitSignal();
    void emitOkSignal();

};

class PathStrokeWidget : public QWidget
{
    Q_OBJECT

public:
    PathStrokeWidget(bool smallScreen);
    void setStyle ( QStyle * style );

private:
    PathStrokeRenderer *m_renderer;
    PathStrokeControls *m_controls;

private slots:
    void showControls();
    void hideControls();
};

#endif // PATHSTROKE_H
