/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#ifndef QOPENGLCOLORMAP_H
#define QOPENGLCOLORMAP_H

#include <QtGui/qcolor.h>
#include <QtCore/qvector.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class Q_GUI_EXPORT QOpenGLColormap
{
public:
    QOpenGLColormap();
    QOpenGLColormap(const QOpenGLColormap &);
    ~QOpenGLColormap();

    QOpenGLColormap &operator=(const QOpenGLColormap &);

    bool   isEmpty() const;
    int    size() const;
    void   detach();

    void   setEntries(int count, const QRgb * colors, int base = 0);
    void   setEntry(int idx, QRgb color);
    void   setEntry(int idx, const QColor & color);
    QRgb   entryRgb(int idx) const;
    QColor entryColor(int idx) const;
    int    find(QRgb color) const;
    int    findNearest(QRgb color) const;

protected:
    Qt::HANDLE handle() { return d ? d->cmapHandle : 0; }
    void setHandle(Qt::HANDLE ahandle) { d->cmapHandle = ahandle; }

private:
    struct QOpenGLColormapData {
        QBasicAtomicInt ref;
        QVector<QRgb> *cells;
        Qt::HANDLE cmapHandle;
    };

    QOpenGLColormapData *d;
    static struct QOpenGLColormapData shared_null;
    static void cleanup(QOpenGLColormapData *x);
    void detach_helper();

    friend class QOpenGLWidget;
    friend class QOpenGLWidgetPrivate;
};

inline void QOpenGLColormap::detach()
{
    if (d->ref != 1)
        detach_helper();
}

QT_END_NAMESPACE

QT_END_HEADER

#endif // QOPENGLCOLORMAP_H
