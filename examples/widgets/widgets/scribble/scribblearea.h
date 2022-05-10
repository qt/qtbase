// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SCRIBBLEAREA_H
#define SCRIBBLEAREA_H

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QWidget>

//! [0]
class ScribbleArea : public QWidget
{
    Q_OBJECT

public:
    ScribbleArea(QWidget *parent = nullptr);

    bool openImage(const QString &fileName);
    bool saveImage(const QString &fileName, const char *fileFormat);
    void setPenColor(const QColor &newColor);
    void setPenWidth(int newWidth);

    bool isModified() const { return modified; }
    QColor penColor() const { return myPenColor; }
    int penWidth() const { return myPenWidth; }

public slots:
    void clearImage();
    void print();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void drawLineTo(const QPoint &endPoint);
    void resizeImage(QImage *image, const QSize &newSize);

    bool modified = false;
    bool scribbling = false;
    int myPenWidth = 1;
    QColor myPenColor = Qt::blue;
    QImage image;
    QPoint lastPoint;
};
//! [0]

#endif
