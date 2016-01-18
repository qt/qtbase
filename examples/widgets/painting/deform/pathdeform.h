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
