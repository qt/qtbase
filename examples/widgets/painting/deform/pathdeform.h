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

#ifndef PATHDEFORM_H
#define PATHDEFORM_H

#include "arthurwidgets.h"

#include <QBasicTimer>
#include <QDateTime>
#include <QPainterPath>

class PathDeformRenderer : public ArthurFrame
{
    Q_OBJECT
    Q_PROPERTY(bool animated READ animated WRITE setAnimated)
    Q_PROPERTY(int radius READ radius WRITE setRadius)
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize)
    Q_PROPERTY(int intensity READ intensity WRITE setIntensity)
    Q_PROPERTY(QString text READ text WRITE setText)

public:
    explicit PathDeformRenderer(QWidget *widget, bool smallScreen = false);

    void paint(QPainter *painter) Q_DECL_OVERRIDE;

    void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void timerEvent(QTimerEvent *e) Q_DECL_OVERRIDE;

    QSize sizeHint() const Q_DECL_OVERRIDE { return QSize(600, 500); }

    bool animated() const { return m_animated; }
    int radius() const { return int(m_radius); }
    int fontSize() const { return m_fontSize; }
    int intensity() const { return int(m_intensity); }
    QString text() const { return m_text; }

public slots:
    void setRadius(int radius);
    void setFontSize(int fontSize) { m_fontSize = fontSize; setText(m_text); }
    void setText(const QString &text);
    void setIntensity(int intensity);

    void setAnimated(bool animated);

signals:
    void clicked();
//     void frameRate(double fps);

private:
    void generateLensPixmap();
    QPainterPath lensDeform(const QPainterPath &source, const QPointF &offset);

    QBasicTimer m_repaintTimer;
//     QBasicTimer m_fpsTimer;
//     int m_fpsCounter;
    QTime m_repaintTracker;

    QVector<QPainterPath> m_paths;
    QVector<QPointF> m_advances;
    QRectF m_pathBounds;
    QString m_text;

    QPixmap m_lens_pixmap;
    QImage m_lens_image;

    int m_fontSize;
    bool m_animated;

    qreal m_intensity;
    qreal m_radius;
    QPointF m_pos;
    QPointF m_offset;
    QPointF m_direction;
    QPointF m_mousePress;
    bool m_mouseDrag;
    bool m_smallScreen;
};

class PathDeformControls : public QWidget
{
    Q_OBJECT
public:
    PathDeformControls(QWidget *parent, PathDeformRenderer* renderer, bool smallScreen);
signals:
    void okPressed();
    void quitPressed();
private:
    PathDeformRenderer* m_renderer;
    void layoutForDesktop();
    void layoutForSmallScreen();
};

class PathDeformWidget : public QWidget
{
    Q_OBJECT
public:
    PathDeformWidget(QWidget *parent, bool smallScreen);
    void setStyle (QStyle * style );

private:
    PathDeformRenderer *m_renderer;
    PathDeformControls *m_controls;

private slots:
    void showControls();
    void hideControls();
};

#endif // PATHDEFORM_H
