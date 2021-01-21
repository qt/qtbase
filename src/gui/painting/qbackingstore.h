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

#ifndef QBACKINGSTORE_H
#define QBACKINGSTORE_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qrect.h>

#include <QtGui/qwindow.h>
#include <QtGui/qregion.h>

QT_BEGIN_NAMESPACE


class QRegion;
class QRect;
class QPoint;
class QImage;
class QBackingStorePrivate;
class QPlatformBackingStore;

class Q_GUI_EXPORT QBackingStore
{
public:
    explicit QBackingStore(QWindow *window);
    ~QBackingStore();

    QWindow *window() const;

    QPaintDevice *paintDevice();

    void flush(const QRegion &region, QWindow *window = nullptr, const QPoint &offset = QPoint());

    void resize(const QSize &size);
    QSize size() const;

    bool scroll(const QRegion &area, int dx, int dy);

    void beginPaint(const QRegion &);
    void endPaint();

    void setStaticContents(const QRegion &region);
    QRegion staticContents() const;
    bool hasStaticContents() const;

    QPlatformBackingStore *handle() const;

private:
    QScopedPointer<QBackingStorePrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QBACKINGSTORE_H
