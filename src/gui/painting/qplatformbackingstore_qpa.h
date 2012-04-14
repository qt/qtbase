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

#ifndef QPLATFORMBACKINGSTORE_QPA_H
#define QPLATFORMBACKINGSTORE_QPA_H

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

#include <QtCore/qrect.h>

#include <QtGui/qwindow.h>
#include <QtGui/qregion.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QRegion;
class QRect;
class QPoint;
class QImage;
class QPlatformBackingStorePrivate;
class QPlatformWindow;

class Q_GUI_EXPORT QPlatformBackingStore
{
public:
    QPlatformBackingStore(QWindow *window);
    virtual ~QPlatformBackingStore();

    QWindow *window() const;

    virtual QPaintDevice *paintDevice() = 0;

    // 'window' can be a child window, in which case 'region' is in child window coordinates and
    // offset is the (child) window's offset in relation to the window surface.
    virtual void flush(QWindow *window, const QRegion &region, const QPoint &offset) = 0;

    virtual void resize(const QSize &size, const QRegion &staticContents) = 0;

    virtual bool scroll(const QRegion &area, int dx, int dy);

    virtual void beginPaint(const QRegion &);
    virtual void endPaint();

private:
    QPlatformBackingStorePrivate *d_ptr;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPLATFORMBACKINGSTORE_QPA_H
