/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QPAGEDPAINTDEVICE_H
#define QPAGEDPAINTDEVICE_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qpaintdevice.h>
#include <QtGui/qpagelayout.h>
#include <QtGui/qpageranges.h>

QT_BEGIN_NAMESPACE

#if defined(B0)
#undef B0 // Terminal hang-up.  We assume that you do not want that.
#endif

class QPagedPaintDevicePrivate;

class Q_GUI_EXPORT QPagedPaintDevice : public QPaintDevice
{
public:
    ~QPagedPaintDevice();

    virtual bool newPage() = 0;

    // keep in sync with QPdfEngine::PdfVersion!
    enum PdfVersion { PdfVersion_1_4, PdfVersion_A1b, PdfVersion_1_6 };

    virtual bool setPageLayout(const QPageLayout &pageLayout);
    virtual bool setPageSize(const QPageSize &pageSize);
    virtual bool setPageOrientation(QPageLayout::Orientation orientation);
    virtual bool setPageMargins(const QMarginsF &margins, QPageLayout::Unit units = QPageLayout::Millimeter);
    QPageLayout pageLayout() const;

    virtual void setPageRanges(const QPageRanges &ranges);
    QPageRanges pageRanges() const;

protected:
    QPagedPaintDevice(QPagedPaintDevicePrivate *dd);
    QPagedPaintDevicePrivate *dd();
    friend class QPagedPaintDevicePrivate;
    QPagedPaintDevicePrivate *d;
};

QT_END_NAMESPACE

#endif
