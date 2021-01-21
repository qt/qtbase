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

#ifndef QRASTERWINDOW_H
#define QRASTERWINDOW_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/QPaintDeviceWindow>

QT_BEGIN_NAMESPACE

class QRasterWindowPrivate;

class Q_GUI_EXPORT QRasterWindow : public QPaintDeviceWindow
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QRasterWindow)

public:
    explicit QRasterWindow(QWindow *parent = nullptr);
    ~QRasterWindow();

protected:
    int metric(PaintDeviceMetric metric) const override;
    QPaintDevice *redirected(QPoint *) const override;

private:
    Q_DISABLE_COPY(QRasterWindow)
};

QT_END_NAMESPACE

#endif
