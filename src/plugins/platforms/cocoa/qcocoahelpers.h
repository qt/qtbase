/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QCOCOAHELPERS_H
#define QCOCOAHELPERS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It provides helper functions
// for the Cocoa lighthouse plugin. This header file may
// change from version to version without notice, or even be removed.
//
// We mean it.
//
#include "qt_mac_p.h"
#include <private/qguiapplication_p.h>
#include <QtGui/qscreen.h>

QT_BEGIN_NAMESPACE

class QPixmap;
class QString;

// Conversion functions
QStringList qt_mac_NSArrayToQStringList(void *nsarray);
void *qt_mac_QStringListToNSMutableArrayVoid(const QStringList &list);

inline NSMutableArray *qt_mac_QStringListToNSMutableArray(const QStringList &qstrlist)
{ return reinterpret_cast<NSMutableArray *>(qt_mac_QStringListToNSMutableArrayVoid(qstrlist)); }

CGImageRef qt_mac_image_to_cgimage(const QImage &image);
NSImage *qt_mac_cgimage_to_nsimage(CGImageRef iamge);
NSImage *qt_mac_create_nsimage(const QPixmap &pm);

NSSize qt_mac_toNSSize(const QSize &qtSize);

QChar qt_mac_qtKey2CocoaKey(Qt::Key key);
Qt::Key qt_mac_cocoaKey2QtKey(QChar keyCode);

NSDragOperation qt_mac_mapDropAction(Qt::DropAction action);
NSDragOperation qt_mac_mapDropActions(Qt::DropActions actions);
Qt::DropAction qt_mac_mapNSDragOperation(NSDragOperation nsActions);
Qt::DropActions qt_mac_mapNSDragOperations(NSDragOperation nsActions);

// Misc
void qt_mac_transformProccessToForegroundApplication();
QString qt_mac_removeMnemonics(const QString &original);
CGColorSpaceRef qt_mac_genericColorSpace();
CGColorSpaceRef qt_mac_displayColorSpace(const QWidget *widget);
QString qt_mac_applicationName();

inline int qt_mac_flipYCoordinate(int y)
{ return QGuiApplication::primaryScreen()->geometry().height() - y; }

inline qreal qt_mac_flipYCoordinate(qreal y)
{ return QGuiApplication::primaryScreen()->geometry().height() - y; }

inline QPointF qt_mac_flipPoint(const NSPoint &p)
{ return QPointF(p.x, qt_mac_flipYCoordinate(p.y)); }

inline NSPoint qt_mac_flipPoint(const QPoint &p)
{ return NSMakePoint(p.x(), qt_mac_flipYCoordinate(p.y())); }

inline NSPoint qt_mac_flipPoint(const QPointF &p)
{ return NSMakePoint(p.x(), qt_mac_flipYCoordinate(p.y())); }

NSRect qt_mac_flipRect(const QRect &rect, QWindow *window);

QT_END_NAMESPACE

#endif //QCOCOAHELPERS_H

