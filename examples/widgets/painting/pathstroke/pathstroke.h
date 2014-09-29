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

    void paint(QPainter *) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void timerEvent(QTimerEvent *e) Q_DECL_OVERRIDE;
    bool event(QEvent *e) Q_DECL_OVERRIDE;

    QSize sizeHint() const Q_DECL_OVERRIDE { return QSize(500, 500); }

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
