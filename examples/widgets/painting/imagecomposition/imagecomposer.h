// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef IMAGECOMPOSER_H
#define IMAGECOMPOSER_H

#include <QPainter>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QToolButton;
QT_END_NAMESPACE

//! [0]
class ImageComposer : public QWidget
{
    Q_OBJECT

public:
    ImageComposer();

private slots:
    void chooseSource();
    void chooseDestination();
    void recalculateResult();
//! [0]

//! [1]
private:
    void addOp(QPainter::CompositionMode mode, const QString &name);
    void chooseImage(const QString &title, QImage *image, QToolButton *button);
    void loadImage(const QString &fileName, QImage *image, QToolButton *button);
    QPainter::CompositionMode currentMode() const;
    QPoint imagePos(const QImage &image) const;

    QToolButton *sourceButton;
    QToolButton *destinationButton;
    QComboBox *operatorComboBox;
    QLabel *equalLabel;
    QLabel *resultLabel;

    QImage sourceImage;
    QImage destinationImage;
    QImage resultImage;
};
//! [1]

#endif // IMAGECOMPOSER_H
