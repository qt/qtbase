// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QA8IMAGE_P_H
#define QA8IMAGE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/qimage.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QA8ImageData : public QSharedData
{
public:
    QA8ImageData() : width(0), height(0), nbytes(0), data(nullptr) {}
    QA8ImageData(const QA8ImageData &other);
    ~QA8ImageData();

    static QA8ImageData *create(QSize size);

    int width;
    int height;
    int nbytes;
    uchar *data;
};

// An 8-bit image with tightly packed scan lines (unlike QImage, which
// aligns scan lines to 4 bytes).
class Q_GUI_EXPORT QA8Image
{
public:
    QA8Image();
    QA8Image(int width, int height);
    explicit QA8Image(QSize size);

    bool isNull() const;

    int width() const;
    int height() const;
    QSize size() const;
    qsizetype sizeInBytes() const;

    void fill(uchar value);

    QA8Image copy(const QRect &rect = QRect()) const;
    inline QA8Image copy(int x, int y, int w, int h) const
        { return copy(QRect(x, y, w, h)); }

    uchar *bits();
    const uchar *bits() const;
    const uchar *constBits() const;

    uchar *scanLine(int);
    const uchar *scanLine(int) const;
    const uchar *constScanLine(int) const;

    QImage toImage(QImage::Format format = QImage::Format_ARGB32_Premultiplied) const;

private:
    QA8Image(QA8ImageData *data);
    QSharedDataPointer<QA8ImageData> d;
};

QT_END_NAMESPACE

#endif // QA8IMAGE_P_H
