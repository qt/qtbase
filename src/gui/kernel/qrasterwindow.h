// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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
    void resizeEvent(QResizeEvent *event) override;

private:
    Q_DISABLE_COPY(QRasterWindow)
};

QT_END_NAMESPACE

#endif
