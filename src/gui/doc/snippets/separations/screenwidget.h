// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SCREENWIDGET_H
#define SCREENWIDGET_H

#include <QColor>
#include <QFrame>
#include <QImage>
#include <QSize>

class QGridLayout;
class QLabel;
class QPushButton;
class QWidget;

class ScreenWidget : public QFrame
{
    Q_OBJECT
public:
    enum Separation { Cyan, Magenta, Yellow };

    ScreenWidget(QWidget *parent, QColor initialColor, const QString &name,
                 Separation mask, const QSize &labelSize);
    void setImage(QImage &image);
    QImage* image();

signals:
    void imageChanged();

public slots:
    void setColor();
    void invertImage();

private:
    void createImage();

    bool inverted;
    QColor paintColor;
    QImage newImage;
    QImage originalImage;
    QLabel *imageLabel;
    QLabel *nameLabel;
    QPushButton *colorButton;
    QPushButton *invertButton;
    Separation maskColor;
};

#endif
