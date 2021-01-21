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

#ifndef QPDFWRITER_H
#define QPDFWRITER_H

#include <QtGui/qtguiglobal.h>

#ifndef QT_NO_PDF

#include <QtCore/qobject.h>
#include <QtGui/qpagedpaintdevice.h>
#include <QtGui/qpagelayout.h>

QT_BEGIN_NAMESPACE

class QIODevice;
class QPdfWriterPrivate;

class Q_GUI_EXPORT QPdfWriter : public QObject, public QPagedPaintDevice
{
    Q_OBJECT
public:
    explicit QPdfWriter(const QString &filename);
    explicit QPdfWriter(QIODevice *device);
    ~QPdfWriter();

    void setPdfVersion(PdfVersion version);
    PdfVersion pdfVersion() const;

    QString title() const;
    void setTitle(const QString &title);

    QString creator() const;
    void setCreator(const QString &creator);

    bool newPage() override;

    void setResolution(int resolution);
    int resolution() const;

    void setDocumentXmpMetadata(const QByteArray &xmpMetadata);
    QByteArray documentXmpMetadata() const;

    void addFileAttachment(const QString &fileName, const QByteArray &data, const QString &mimeType = QString());

#ifdef Q_QDOC
    bool setPageLayout(const QPageLayout &pageLayout);
    bool setPageSize(const QPageSize &pageSize);
    bool setPageOrientation(QPageLayout::Orientation orientation);
    bool setPageMargins(const QMarginsF &margins);
    bool setPageMargins(const QMarginsF &margins, QPageLayout::Unit units);
    QPageLayout pageLayout() const;
#else
    using QPagedPaintDevice::setPageSize;
#endif

#if QT_DEPRECATED_SINCE(5, 14)
    QT_DEPRECATED_X("Use setPageSize(QPageSize(id)) instead")
    void setPageSize(PageSize size) override;
    QT_DEPRECATED_X("Use setPageSize(QPageSize(size, QPageSize::Millimeter)) instead")
    void setPageSizeMM(const QSizeF &size) override;
    QT_DEPRECATED_X("Use setPageMargins(QMarginsF(l, t, r, b), QPageLayout::Millimeter) instead")
    void setMargins(const Margins &m) override;
#endif

protected:
    QPaintEngine *paintEngine() const override;
    int metric(PaintDeviceMetric id) const override;

private:
    Q_DISABLE_COPY(QPdfWriter)
    Q_DECLARE_PRIVATE(QPdfWriter)
};

QT_END_NAMESPACE

#endif // QT_NO_PDF

#endif
