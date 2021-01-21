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

#ifndef QBITMAP_H
#define QBITMAP_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qpixmap.h>

QT_BEGIN_NAMESPACE


class QVariant;

class Q_GUI_EXPORT QBitmap : public QPixmap
{
public:
    QBitmap();
    QBitmap(const QPixmap &);
    QBitmap(int w, int h);
    explicit QBitmap(const QSize &);
    explicit QBitmap(const QString &fileName, const char *format = nullptr);
    // ### Qt 6: don't inherit QPixmap
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QBitmap(const QBitmap &other) : QPixmap(other) {}
    // QBitmap(QBitmap &&other) : QPixmap(std::move(other)) {} // QPixmap doesn't, yet, have a move ctor
    QBitmap &operator=(const QBitmap &other) { QPixmap::operator=(other); return *this; }
    QBitmap &operator=(QBitmap &&other) noexcept { QPixmap::operator=(std::move(other)); return *this; }
    ~QBitmap();
#endif

    QBitmap &operator=(const QPixmap &);
    inline void swap(QBitmap &other) { QPixmap::swap(other); } // prevent QBitmap<->QPixmap swaps
    operator QVariant() const;

    inline void clear() { fill(Qt::color0); }

    static QBitmap fromImage(const QImage &image, Qt::ImageConversionFlags flags = Qt::AutoColor);
    static QBitmap fromImage(QImage &&image, Qt::ImageConversionFlags flags = Qt::AutoColor);
    static QBitmap fromData(const QSize &size, const uchar *bits,
                            QImage::Format monoFormat = QImage::Format_MonoLSB);

#if QT_DEPRECATED_SINCE(5, 13)
    QT_DEPRECATED_X("Use QBitmap::transformed(QTransform) instead")
    QBitmap transformed(const QMatrix &) const;
#endif
    QBitmap transformed(const QTransform &matrix) const;

    typedef QExplicitlySharedDataPointer<QPlatformPixmap> DataPtr;
};
Q_DECLARE_SHARED(QBitmap)

QT_END_NAMESPACE

#endif // QBITMAP_H
