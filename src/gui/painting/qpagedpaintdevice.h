/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPAGEDPAINTDEVICE_H
#define QPAGEDPAINTDEVICE_H

#include <QtGui/qpaintdevice.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QPagedPaintDevicePrivate;

class Q_GUI_EXPORT QPagedPaintDevice : public QPaintDevice
{
public:
    QPagedPaintDevice();
    ~QPagedPaintDevice();

    virtual bool newPage() = 0;

    enum PageSize { A4, B5, Letter, Legal, Executive,
                    A0, A1, A2, A3, A5, A6, A7, A8, A9, B0, B1,
                    B10, B2, B3, B4, B6, B7, B8, B9, C5E, Comm10E,
                    DLE, Folio, Ledger, Tabloid, Custom, NPageSize = Custom };

    virtual void setPageSize(PageSize size);
    PageSize pageSize() const;

    virtual void setPageSizeMM(const QSizeF &size);
    QSizeF pageSizeMM() const;

    struct Margins {
        qreal left;
        qreal right;
        qreal top;
        qreal bottom;
    };

    virtual void setMargins(const Margins &margins);
    Margins margins() const;

protected:
    friend class QPagedPaintDevicePrivate;
    QPagedPaintDevicePrivate *d;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif
