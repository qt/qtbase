/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPDFWRITER_H
#define QPDFWRITER_H

#include <QtCore/qobject.h>
#include <QtGui/qpagedpaintdevice.h>

QT_BEGIN_HEADER

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

    QString title() const;
    void setTitle(const QString &title);

    QString creator() const;
    void setCreator(const QString &creator);

    bool newPage();

    void setPageSize(PageSize size);
    void setPageSizeMM(const QSizeF &size);

    void setMargins(const Margins &m);

protected:
    QPaintEngine *paintEngine() const;
    int metric(PaintDeviceMetric id) const;

private:
    Q_DISABLE_COPY(QPdfWriter)
    Q_DECLARE_PRIVATE(QPdfWriter)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif
