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

#ifndef QPLATFORMSCREEN_H
#define QPLATFORMSCREEN_H

#include <QtCore/qmetatype.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qvariant.h>
#include <QtCore/qrect.h>
#include <QtCore/qobject.h>

#include <QtGui/qcursor.h>
#include <QtGui/qimage.h>
#include <QtGui/qwindowdefs.h>
#include <QtGui/qplatformpixmap_qpa.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QPlatformBackingStore;
class QPlatformOpenGLContext;
class QPlatformScreenPrivate;
class QPlatformWindow;
class QPlatformCursor;
class QPlatformScreenPageFlipper;
class QScreen;
class QSurfaceFormat;

typedef QPair<qreal, qreal> QDpi;


class Q_GUI_EXPORT QPlatformScreen
{
    Q_DECLARE_PRIVATE(QPlatformScreen)

public:
    QPlatformScreen();
    virtual ~QPlatformScreen();

    virtual QPixmap grabWindow(WId window, int x, int y, int width, int height) const;

    virtual QRect geometry() const = 0;
    virtual QRect availableGeometry() const {return geometry();}

    virtual int depth() const = 0;
    virtual QImage::Format format() const = 0;

    virtual QSizeF physicalSize() const;
    virtual QDpi logicalDpi() const;

    virtual Qt::ScreenOrientation orientation() const;

    virtual QWindow *topLevelAt(const QPoint &point) const;
    virtual QList<QPlatformScreen *> virtualSiblings() const;

    QScreen *screen() const;

    //jl: should this function be in QPlatformIntegration
    //jl: maybe screenForWindow is a better name?
    static QPlatformScreen *platformScreenForWindow(const QWindow *window);

    virtual QString name() const { return QString(); }

    virtual QPlatformScreenPageFlipper *pageFlipper() const;
    virtual QPlatformCursor *cursor() const;

protected:
    QScopedPointer<QPlatformScreenPrivate> d_ptr;

private:
    Q_DISABLE_COPY(QPlatformScreen)

    friend class QPlatformIntegration;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPLATFORMSCREEN_H
