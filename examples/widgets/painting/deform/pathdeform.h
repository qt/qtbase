// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef PATHDEFORM_H
#define PATHDEFORM_H

#include "arthurwidgets.h"

#include <QBasicTimer>
#include <QElapsedTimer>
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

    void paint(QPainter *painter) override;

    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void timerEvent(QTimerEvent *e) override;

    QSize sizeHint() const override { return QSize(600, 500); }

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
    QElapsedTimer m_repaintTracker;

    QList<QPainterPath> m_paths;
    QList<QPointF> m_advances;
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
    PathDeformRenderer *m_renderer;
    void layoutForDesktop();
    void layoutForSmallScreen();
};

class PathDeformWidget : public QWidget
{
    Q_OBJECT
public:
    PathDeformWidget(QWidget *parent, bool smallScreen);
    void setStyle(QStyle *style);

private:
    PathDeformRenderer *m_renderer;
    PathDeformControls *m_controls;

private slots:
    void showControls();
    void hideControls();
};

#endif // PATHDEFORM_H
