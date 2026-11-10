// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QPAGEDPAINTDEVICE_H
#define QPAGEDPAINTDEVICE_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qpaintdevice.h>
#include <QtGui/qpagelayout.h>
#include <QtGui/qpageranges.h>

QT_BEGIN_NAMESPACE

class QPagedPaintDevicePrivate;

class Q_GUI_EXPORT QPagedPaintDevice : public QPaintDevice
{
public:
    ~QPagedPaintDevice();

    virtual bool newPage() = 0;

    // keep in sync with QPdfEngine::PdfVersion!
    enum PdfVersion {
        PdfVersion_1_4,
        PdfVersion_A1b,
        PdfVersion_1_6,
        PdfVersion_X4,
        // bump NumPdfVersions when adding new enumerators
    };
    static inline constexpr size_t NumPdfVersions = 4;

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
