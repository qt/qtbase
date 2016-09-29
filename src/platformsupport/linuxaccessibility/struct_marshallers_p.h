/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef Q_SPI_STRUCT_MARSHALLERS_H
#define Q_SPI_STRUCT_MARSHALLERS_H

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
#include <QtCore/qvector.h>
#include <QtCore/qpair.h>
#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusObjectPath>

QT_REQUIRE_CONFIG(accessibility);

QT_BEGIN_NAMESPACE

typedef QVector<int> QSpiIntList;
typedef QVector<uint> QSpiUIntList;

// FIXME: make this copy on write
struct QSpiObjectReference
{
    QString service;
    QDBusObjectPath path;

    QSpiObjectReference();
    QSpiObjectReference(const QDBusConnection& connection, const QDBusObjectPath& path)
        : service(connection.baseService()), path(path) {}
};
Q_DECLARE_TYPEINFO(QSpiObjectReference, Q_MOVABLE_TYPE); // QDBusObjectPath is movable, even though it
                                                         // cannot be marked that way until Qt 6

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiObjectReference &address);
const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiObjectReference &address);

typedef QVector<QSpiObjectReference> QSpiObjectReferenceArray;

struct QSpiAccessibleCacheItem
{
    QSpiObjectReference         path;
    QSpiObjectReference         application;
    QSpiObjectReference         parent;
    QSpiObjectReferenceArray children;
    QStringList                 supportedInterfaces;
    QString                     name;
    uint                        role;
    QString                     description;
    QSpiUIntList                state;
};
Q_DECLARE_TYPEINFO(QSpiAccessibleCacheItem, Q_MOVABLE_TYPE);

typedef QVector<QSpiAccessibleCacheItem> QSpiAccessibleCacheArray;

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiAccessibleCacheItem &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiAccessibleCacheItem &item);

struct QSpiAction
{
    QString name;
    QString description;
    QString keyBinding;
};
Q_DECLARE_TYPEINFO(QSpiAction, Q_MOVABLE_TYPE);

typedef QVector<QSpiAction> QSpiActionArray;

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiAction &action);
const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiAction &action);

struct QSpiEventListener
{
    QString listenerAddress;
    QString eventName;
};
Q_DECLARE_TYPEINFO(QSpiEventListener, Q_MOVABLE_TYPE);

typedef QVector<QSpiEventListener> QSpiEventListenerArray;

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiEventListener &action);
const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiEventListener &action);

typedef QPair<unsigned int, QSpiObjectReferenceArray> QSpiRelationArrayEntry;
typedef QVector<QSpiRelationArrayEntry> QSpiRelationArray;

//a(iisv)
struct QSpiTextRange {
    int startOffset;
    int endOffset;
    QString contents;
    QVariant v;
};
Q_DECLARE_TYPEINFO(QSpiTextRange, Q_MOVABLE_TYPE);

typedef QVector<QSpiTextRange> QSpiTextRangeList;
typedef QMap <QString, QString> QSpiAttributeSet;

enum QSpiAppUpdateType {
    QSPI_APP_UPDATE_ADDED = 0,
    QSPI_APP_UPDATE_REMOVED = 1
};
Q_DECLARE_TYPEINFO(QSpiAppUpdateType, Q_PRIMITIVE_TYPE);

struct QSpiAppUpdate {
    int type; /* Is an application added or removed */
    QString address; /* D-Bus address of application added or removed */
};
Q_DECLARE_TYPEINFO(QSpiAppUpdate, Q_MOVABLE_TYPE);

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiAppUpdate &update);
const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiAppUpdate &update);

struct QSpiDeviceEvent {
    unsigned int type;
    int id;
    int hardwareCode;
    int modifiers;
    int timestamp;
    QString text;
    bool isText;
};
Q_DECLARE_TYPEINFO(QSpiDeviceEvent, Q_MOVABLE_TYPE);

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiDeviceEvent &event);
const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiDeviceEvent &event);

void qSpiInitializeStructTypes();

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QSpiIntList)
Q_DECLARE_METATYPE(QSpiUIntList)
Q_DECLARE_METATYPE(QSpiObjectReference)
Q_DECLARE_METATYPE(QSpiObjectReferenceArray)
Q_DECLARE_METATYPE(QSpiAccessibleCacheItem)
Q_DECLARE_METATYPE(QSpiAccessibleCacheArray)
Q_DECLARE_METATYPE(QSpiAction)
Q_DECLARE_METATYPE(QSpiActionArray)
Q_DECLARE_METATYPE(QSpiEventListener)
Q_DECLARE_METATYPE(QSpiEventListenerArray)
Q_DECLARE_METATYPE(QSpiRelationArrayEntry)
Q_DECLARE_METATYPE(QSpiRelationArray)
Q_DECLARE_METATYPE(QSpiTextRange)
Q_DECLARE_METATYPE(QSpiTextRangeList)
Q_DECLARE_METATYPE(QSpiAttributeSet)
Q_DECLARE_METATYPE(QSpiAppUpdate)
Q_DECLARE_METATYPE(QSpiDeviceEvent)

#endif /* Q_SPI_STRUCT_MARSHALLERS_H */
