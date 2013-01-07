/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef ATSPIADAPTOR_H
#define ATSPIADAPTOR_H

#include <atspi/atspi-constants.h>

#include <QtCore/qsharedpointer.h>
#include <QtDBus/qdbusvirtualobject.h>
#include <QtGui/qaccessible.h>
#include <QtGui/qaccessible2.h>

#include "dbusconnection_p.h"
#include "struct_marshallers_p.h"

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

class QAccessibleInterface;
class QSpiAccessibleInterface;
class QSpiApplicationAdaptor;

typedef QSharedPointer<QAccessibleInterface> QAIPointer;

class AtSpiAdaptor :public QDBusVirtualObject
{
    Q_OBJECT

public:
    explicit AtSpiAdaptor(DBusConnection *connection, QObject *parent = 0);
    ~AtSpiAdaptor();

    void registerApplication();
    QString introspect(const QString &path) const;
    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection);
    void notify(QAccessibleEvent *event);

    void setInitialized(bool init);

public Q_SLOTS:
    void eventListenerRegistered(const QString &bus, const QString &path);
    void eventListenerDeregistered(const QString &bus, const QString &path);
    void windowActivated(QObject* window, bool active);

private:
    void updateEventListeners();
    void setBitFlag(const QString &flag);

    // sending messages
    QVariantList packDBusSignalArguments(const QString &type, int data1, int data2, const QVariant &variantData) const;
    bool sendDBusSignal(const QString &path, const QString &interface, const QString &name, const QVariantList &arguments) const;
    QVariant variantForPath(const QString &path) const;

    void sendFocusChanged(const QAIPointer &interface) const;
    void notifyAboutCreation(const QAIPointer &interface) const;
    void notifyAboutDestruction(const QAIPointer &interface) const;

    // handlers for the different accessible interfaces
    bool applicationInterface(const QAIPointer &interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection);
    bool accessibleInterface(const QAIPointer &interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection);
    bool componentInterface(const QAIPointer &interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection);
    bool actionInterface(const QAIPointer &interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection);
    bool textInterface(const QAIPointer &interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection);
    bool editableTextInterface(const QAIPointer &interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection);
    bool valueInterface(const QAIPointer &interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection);
    bool tableInterface(const QAIPointer &interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection);

    void sendReply(const QDBusConnection &connection, const QDBusMessage &message, const QVariant &argument) const;

    QAIPointer interfaceFromPath(const QString& dbusPath) const;
    QString pathForInterface(const QAIPointer &interface, bool inDestructor = false) const;
    QString pathForObject(QObject *object) const;

    void notifyStateChange(const QAIPointer& interface, const QString& state, int value);

    // accessible helper functions
    AtspiRole getRole(const QAIPointer &interface) const;
    QSpiRelationArray relationSet(const QAIPointer &interface, const QDBusConnection &connection) const;
    QStringList accessibleInterfaces(const QAIPointer &interface) const;

    // component helper functions
    static QRect getExtents(const QAIPointer &interface, uint coordType);
    static QRect translateRectToWindowCoordinates(const QAIPointer &interface, const QRect &rect);

    // action helper functions
    QSpiActionArray getActions(QAccessibleActionInterface* interface) const;

    // text helper functions
    QVariantList getAttributes(const QAIPointer &, int offset, bool includeDefaults) const;
    QVariantList getAttributeValue(const QAIPointer &, int offset, const QString &attributeName) const;
    QRect getCharacterExtents(const QAIPointer &, int offset, uint coordType) const;
    QRect getRangeExtents(const QAIPointer &, int startOffset, int endOffset, uint coordType) const;
    QAccessible2::BoundaryType qAccessibleBoundaryType(int atspiTextBoundaryType) const;
    static bool inheritsQAction(QObject *object);

    // private vars
    QSpiObjectReference accessibilityRegistry;
    DBusConnection *m_dbus;
    QSpiApplicationAdaptor *m_applicationAdaptor;

    /// Assigned from the accessibility registry.
    int m_applicationId;
    bool initialized;

    mutable QHash<quintptr, QPointer<QObject> > m_handledObjects;

    // Bit fields - which updates to send

    // AT-SPI has some events that we do not care about:
    // document
    // document-load-complete
    // document-load-stopped
    // document-reload
    uint sendFocus : 1;
    // mouse abs/rel/button

    // all of object
    uint sendObject : 1;
    uint sendObject_active_descendant_changed : 1;
    uint sendObject_attributes_changed : 1;
    uint sendObject_bounds_changed : 1;
    uint sendObject_children_changed : 1;
//    uint sendObject_children_changed_add : 1;
//    uint sendObject_children_changed_remove : 1;
    uint sendObject_column_deleted : 1;
    uint sendObject_column_inserted : 1;
    uint sendObject_column_reordered : 1;
    uint sendObject_link_selected : 1;
    uint sendObject_model_changed : 1;
    uint sendObject_property_change : 1;
    uint sendObject_property_change_accessible_description : 1;
    uint sendObject_property_change_accessible_name : 1;
    uint sendObject_property_change_accessible_parent : 1;
    uint sendObject_property_change_accessible_role : 1;
    uint sendObject_property_change_accessible_table_caption : 1;
    uint sendObject_property_change_accessible_table_column_description : 1;
    uint sendObject_property_change_accessible_table_column_header : 1;
    uint sendObject_property_change_accessible_table_row_description : 1;
    uint sendObject_property_change_accessible_table_row_header : 1;
    uint sendObject_property_change_accessible_table_summary : 1;
    uint sendObject_property_change_accessible_value : 1;
    uint sendObject_row_deleted : 1;
    uint sendObject_row_inserted : 1;
    uint sendObject_row_reordered : 1;
    uint sendObject_selection_changed : 1;
    uint sendObject_state_changed : 1;
    uint sendObject_text_attributes_changed : 1;
    uint sendObject_text_bounds_changed : 1;
    uint sendObject_text_caret_moved : 1;
    uint sendObject_text_changed : 1;
//    uint sendObject_text_changed_delete : 1;
//    uint sendObject_text_changed_insert : 1;
    uint sendObject_text_selection_changed : 1;
    uint sendObject_value_changed : 1;
    uint sendObject_visible_data_changed : 1;

    // we don't implement terminal
    // terminal-application_changed/charwidth_changed/columncount_changed/line_changed/linecount_changed
    uint sendWindow : 1;
    uint sendWindow_activate : 1;
    uint sendWindow_close: 1;
    uint sendWindow_create : 1;
    uint sendWindow_deactivate : 1;
//    uint sendWindow_desktop_create : 1;
//    uint sendWindow_desktop_destroy : 1;
    uint sendWindow_lower : 1;
    uint sendWindow_maximize : 1;
    uint sendWindow_minimize : 1;
    uint sendWindow_move : 1;
    uint sendWindow_raise : 1;
    uint sendWindow_reparent : 1;
    uint sendWindow_resize : 1;
    uint sendWindow_restore : 1;
    uint sendWindow_restyle : 1;
    uint sendWindow_shade : 1;
    uint sendWindow_unshade : 1;
};

QT_END_NAMESPACE
QT_END_HEADER

#endif
