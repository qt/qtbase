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

    bool isModified() const { return modified; }

public slots:
    void clearImage();
    void print();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool event(QEvent *event) override;

private:
    void resizeImage(QImage *image, const QSize &newSize);

    bool modified;
    QList<QColor> myPenColors;
    QImage image;
};
//! [0]

#endif
