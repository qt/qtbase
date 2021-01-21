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

#ifndef QPAGEDPAINTDEVICE_P_H
#define QPAGEDPAINTDEVICE_P_H

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
#include <qpagedpaintdevice.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QPagedPaintDevicePrivate
{
public:
    QPagedPaintDevicePrivate()
        : fromPage(0),
          toPage(0),
          pageOrderAscending(true),
          printSelectionOnly(false)
    {
    }

    virtual ~QPagedPaintDevicePrivate();


    virtual bool setPageLayout(const QPageLayout &newPageLayout) = 0;

    virtual bool setPageSize(const QPageSize &pageSize) = 0;

    virtual bool setPageOrientation(QPageLayout::Orientation orientation) = 0;

    virtual bool setPageMargins(const QMarginsF &margins, QPageLayout::Unit units) = 0;

    virtual QPageLayout pageLayout() const = 0;

    static inline QPagedPaintDevicePrivate *get(QPagedPaintDevice *pd) { return pd->d; }

    // These are currently required to keep QPrinter functionality working in QTextDocument::print()
    int fromPage;
    int toPage;
    bool pageOrderAscending;
    bool printSelectionOnly;
};

QT_END_NAMESPACE

#endif
