/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QPAINTDEVICEWINDOW_H
#define QPAINTDEVICEWINDOW_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/QWindow>
#include <QtGui/QPaintDevice>

QT_BEGIN_NAMESPACE

class QPaintDeviceWindowPrivate;
class QPaintEvent;

class Q_GUI_EXPORT QPaintDeviceWindow : public QWindow, public QPaintDevice
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPaintDeviceWindow)

public:
    void update(const QRect &rect);
    void update(const QRegion &region);

    using QWindow::width;
    using QWindow::height;
    using QWindow::devicePixelRatio;

public Q_SLOTS:
    void update();

protected:
    virtual void paintEvent(QPaintEvent *event);

    int metric(PaintDeviceMetric metric) const override;
    void exposeEvent(QExposeEvent *) override;
    bool event(QEvent *event) override;

    QPaintDeviceWindow(QPaintDeviceWindowPrivate &dd, QWindow *parent);

private:
    QPaintEngine *paintEngine() const override;
    Q_DISABLE_COPY(QPaintDeviceWindow)
};

QT_END_NAMESPACE

#endif
