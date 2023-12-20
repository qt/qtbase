// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef IMAGEWIDGET_H
#define IMAGEWIDGET_H

#include <QFileInfo>
#include <QImage>
#include <QLoggingCategory>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QGestureEvent;
class QPanGesture;
class QPinchGesture;
class QSwipeGesture;
QT_END_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcExample)

//! [class definition begin]
class ImageWidget : public QWidget
{
    Q_OBJECT

public:
    ImageWidget(QWidget *parent = nullptr);
    void openDirectory(const QString &url);
    void grabGestures(const QList<Qt::GestureType> &gestures);

protected:
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    bool gestureEvent(QGestureEvent *event);
    void panTriggered(QPanGesture*);
    void pinchTriggered(QPinchGesture*);
    void swipeTriggered(QSwipeGesture*);
//! [class definition begin]

    QImage loadImage(const QFileInfo &fileInfo) const;
    void loadImage();
    void goNextImage();
    void goPrevImage();
    void goToImage(int index);

    QString path;
    QFileInfoList files;
    int position;

    QImage prevImage, nextImage;
    QImage currentImage;

    qreal horizontalOffset;
    qreal verticalOffset;
    qreal rotationAngle;
    qreal scaleFactor;
    qreal currentStepScaleFactor;
//! [class definition end]
};
//! [class definition end]

#endif
