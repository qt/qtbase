// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "atspiadaptor_p.h"
#include "qspiaccessiblebridge_p.h"

#include <QtGui/qwindow.h>
#include <QtGui/qguiapplication.h>
#include <qdbusmessage.h>
#include <qdbusreply.h>
#include <qclipboard.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qtversion.h>

#if QT_CONFIG(accessibility)
#include "socket_interface.h"
#include "qspi_constant_mappings_p.h"
#include <QtCore/private/qstringiterator_p.h>
#include <QtGui/private/qaccessiblebridgeutils_p.h>

#include "qspiapplicationadaptor_p.h"
/*!
    \class AtSpiAdaptor
    \internal

    \brief AtSpiAdaptor is the main class to forward between QAccessibleInterface and AT-SPI DBus

    AtSpiAdaptor implements the functions specified in all at-spi interfaces.
    It sends notifications coming from Qt via dbus and listens to incoming dbus requests.
*/

// ATSPI_COORD_TYPE_PARENT was added in at-spi 2.30, define here for older versions
#if ATSPI_COORD_TYPE_COUNT < 3
#define ATSPI_COORD_TYPE_PARENT 2
#endif

// ATSPI_*_VERSION defines were added in libatspi 2.50,
// as was the AtspiLive enum; define values here for older versions
#if !defined(ATSPI_MAJOR_VERSION) || !defined(ATSPI_MINOR_VERSION) || ATSPI_MAJOR_VERSION < 2 || ATSPI_MINOR_VERSION < 50
#define ATSPI_LIVE_POLITE 1
#define ATSPI_LIVE_ASSERTIVE 2
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QtGuiPrivate; // for D-Bus accessibility wrappers

Q_STATIC_LOGGING_CATEGORY(lcAccessibilityAtspi, "qt.accessibility.atspi")
Q_STATIC_LOGGING_CATEGORY(lcAccessibilityAtspiCreation, "qt.accessibility.atspi.creation")

AtSpiAdaptor::AtSpiAdaptor(QAtSpiDBusConnection *connection, QObject *parent)
    : QDBusVirtualObject(parent), m_dbus(connection)
    , sendFocus(0)
    , sendObject(0)
    , sendObject_active_descendant_changed(0)
    , sendObject_announcement(0)
    , sendObject_attributes_changed(0)
    , sendObject_bounds_changed(0)
    , sendObject_children_changed(0)
//    , sendObject_children_changed_add(0)
//    , sendObject_children_changed_remove(0)
    , sendObject_column_deleted(0)
    , sendObject_column_inserted(0)
    , sendObject_column_reordered(0)
    , sendObject_link_selected(0)
    , sendObject_model_changed(0)
    , sendObject_property_change(0)
    , sendObject_property_change_accessible_description(0)
    , sendObject_property_change_accessible_name(0)
    , sendObject_property_change_accessible_parent(0)
    , sendObject_property_change_accessible_role(0)
    , sendObject_property_change_accessible_table_caption(0)
    , sendObject_property_change_accessible_table_column_description(0)
    , sendObject_property_change_accessible_table_column_header(0)
    , sendObject_property_change_accessible_table_row_description(0)
    , sendObject_property_change_accessible_table_row_header(0)
    , sendObject_property_change_accessible_table_summary(0)
    , sendObject_property_change_accessible_value(0)
    , sendObject_row_deleted(0)
    , sendObject_row_inserted(0)
    , sendObject_row_reordered(0)
    , sendObject_selection_changed(0)
    , sendObject_state_changed(0)
    , sendObject_text_attributes_changed(0)
    , sendObject_text_bounds_changed(0)
    , sendObject_text_caret_moved(0)
    , sendObject_text_changed(0)
//    , sendObject_text_changed_delete(0)
//    , sendObject_text_changed_insert(0)
    , sendObject_text_selection_changed(0)
    , sendObject_value_changed(0)
    , sendObject_visible_data_changed(0)
    , sendWindow(0)
    , sendWindow_activate(0)
    , sendWindow_close(0)
    , sendWindow_create(0)
    , sendWindow_deactivate(0)
//    , sendWindow_desktop_create(0)
//    , sendWindow_desktop_destroy(0)
    , sendWindow_lower(0)
    , sendWindow_maximize(0)
    , sendWindow_minimize(0)
    , sendWindow_move(0)
    , sendWindow_raise(0)
    , sendWindow_reparent(0)
    , sendWindow_resize(0)
    , sendWindow_restore(0)
    , sendWindow_restyle(0)
    , sendWindow_shade(0)
    , sendWindow_unshade(0)
{
    m_applicationAdaptor = new QSpiApplicationAdaptor(m_dbus->connection(), this);
    connect(m_applicationAdaptor, SIGNAL(windowActivated(QObject*,bool)), this, SLOT(windowActivated(QObject*,bool)));

    updateEventListeners();
    bool success = m_dbus->connection().connect("org.a11y.atspi.Registry"_L1, "/org/a11y/atspi/registry"_L1,
                                                "org.a11y.atspi.Registry"_L1, "EventListenerRegistered"_L1, this,
                                                SLOT(eventListenerRegistered(QString,QString)));
    success = success && m_dbus->connection().connect("org.a11y.atspi.Registry"_L1, "/org/a11y/atspi/registry"_L1,
                                                      "org.a11y.atspi.Registry"_L1, "EventListenerDeregistered"_L1, this,
                                                      SLOT(eventListenerDeregistered(QString,QString)));
}

AtSpiAdaptor::~AtSpiAdaptor()
{
}

/*!
  Provide DBus introspection.
  */
QString AtSpiAdaptor::introspect(const QString &path) const
{
    static const QLatin1StringView accessibleIntrospection(
                "  <interface name=\"org.a11y.atspi.Accessible\">\n"
                "    <property access=\"read\" type=\"s\" name=\"Name\"/>\n"
                "    <property access=\"read\" type=\"s\" name=\"Description\"/>\n"
                "    <property access=\"read\" type=\"s\" name=\"HelpText\"/>\n"
                "    <property access=\"read\" type=\"(so)\" name=\"Parent\">\n"
                "      <annotation value=\"QSpiObjectReference\" name=\"org.qtproject.QtDBus.QtTypeName\"/>\n"
                "    </property>\n"
                "    <property access=\"read\" type=\"i\" name=\"ChildCount\"/>\n"
                "    <method name=\"GetChildAtIndex\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"index\"/>\n"
                "      <arg direction=\"out\" type=\"(so)\"/>\n"
                "      <annotation value=\"QSpiObjectReference\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetChildren\">\n"
                "      <arg direction=\"out\" type=\"a(so)\"/>\n"
                "      <annotation value=\"QSpiObjectReferenceArray\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetIndexInParent\">\n"
                "      <arg direction=\"out\" type=\"i\"/>\n"
                "    </method>\n"
                "    <method name=\"GetRelationSet\">\n"
                "      <arg direction=\"out\" type=\"a(ua(so))\"/>\n"
                "      <annotation value=\"QSpiRelationArray\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetRole\">\n"
                "      <arg direction=\"out\" type=\"u\"/>\n"
                "    </method>\n"
                "    <method name=\"GetRoleName\">\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "    </method>\n"
                "    <method name=\"GetLocalizedRoleName\">\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "    </method>\n"
                "    <method name=\"GetState\">\n"
                "      <arg direction=\"out\" type=\"au\"/>\n"
                "      <annotation value=\"QSpiUIntList\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetAttributes\">\n"
                "      <arg direction=\"out\" type=\"a{ss}\"/>\n"
                "      <annotation value=\"QSpiAttributeSet\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetApplication\">\n"
                "      <arg direction=\"out\" type=\"(so)\"/>\n"
                "      <annotation value=\"QSpiObjectReference\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetAccessibleId\">\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                );

    static const QLatin1StringView actionIntrospection(
                "  <interface name=\"org.a11y.atspi.Action\">\n"
                "    <property access=\"read\" type=\"i\" name=\"NActions\"/>\n"
                "    <method name=\"GetDescription\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"index\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "    </method>\n"
                "    <method name=\"GetName\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"index\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "    </method>\n"
                "    <method name=\"GetKeyBinding\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"index\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "    </method>\n"
                "    <method name=\"GetActions\">\n"
                "      <arg direction=\"out\" type=\"a(sss)\" name=\"index\"/>\n"
                "      <annotation value=\"QSpiActionArray\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"DoAction\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"index\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                );

    static const QLatin1StringView applicationIntrospection(
                "  <interface name=\"org.a11y.atspi.Application\">\n"
                "    <property access=\"read\" type=\"s\" name=\"ToolkitName\"/>\n"
                "    <property access=\"read\" type=\"s\" name=\"Version\"/>\n"
                "    <property access=\"readwrite\" type=\"i\" name=\"Id\"/>\n"
                "    <method name=\"GetLocale\">\n"
                "      <arg direction=\"in\" type=\"u\" name=\"lctype\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "    </method>\n"
                "    <method name=\"GetApplicationBusAddress\">\n"
                "      <arg direction=\"out\" type=\"s\" name=\"address\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                );

    static const QLatin1StringView componentIntrospection(
                "  <interface name=\"org.a11y.atspi.Component\">\n"
                "    <method name=\"Contains\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"x\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"y\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"coord_type\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"GetAccessibleAtPoint\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"x\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"y\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"coord_type\"/>\n"
                "      <arg direction=\"out\" type=\"(so)\"/>\n"
                "      <annotation value=\"QSpiObjectReference\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetExtents\">\n"
                "      <arg direction=\"in\" type=\"u\" name=\"coord_type\"/>\n"
                "      <arg direction=\"out\" type=\"(iiii)\"/>\n"
                "      <annotation value=\"QSpiRect\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetPosition\">\n"
                "      <arg direction=\"in\" type=\"u\" name=\"coord_type\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"x\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"y\"/>\n"
                "    </method>\n"
                "    <method name=\"GetSize\">\n"
                "      <arg direction=\"out\" type=\"i\" name=\"width\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"height\"/>\n"
                "    </method>\n"
                "    <method name=\"GetLayer\">\n"
                "      <arg direction=\"out\" type=\"u\"/>\n"
                "    </method>\n"
                "    <method name=\"GetMDIZOrder\">\n"
                "      <arg direction=\"out\" type=\"n\"/>\n"
                "    </method>\n"
                "    <method name=\"GrabFocus\">\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"GetAlpha\">\n"
                "      <arg direction=\"out\" type=\"d\"/>\n"
                "    </method>\n"
                "    <method name=\"SetExtents\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"x\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"y\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"width\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"height\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"coord_type\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"SetPosition\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"x\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"y\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"coord_type\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"SetSize\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"width\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"height\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                );

    static const QLatin1StringView editableTextIntrospection(
                "  <interface name=\"org.a11y.atspi.EditableText\">\n"
                "    <method name=\"SetTextContents\">\n"
                "      <arg direction=\"in\" type=\"s\" name=\"newContents\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"InsertText\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"position\"/>\n"
                "      <arg direction=\"in\" type=\"s\" name=\"text\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"length\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"CopyText\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"startPos\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"endPos\"/>\n"
                "    </method>\n"
                "    <method name=\"CutText\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"startPos\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"endPos\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"DeleteText\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"startPos\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"endPos\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"PasteText\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"position\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                );

    static const QLatin1StringView selectionIntrospection(
                "  <interface name=\"org.a11y.atspi.Selection\">\n"
                "    <property name=\"NSelectedChildren\" type=\"i\" access=\"read\"/>\n"
                "    <method name=\"GetSelectedChild\">\n"
                "      <arg direction=\"in\" name=\"selectedChildIndex\" type=\"i\"/>\n"
                "      <arg direction=\"out\" type=\"(so)\"/>\n"
                "      <annotation name=\"org.qtproject.QtDBus.QtTypeName.Out0\" value=\"QSpiObjectReference\"/>\n"
                "    </method>\n"
                "    <method name=\"SelectChild\">\n"
                "      <arg direction=\"in\" name=\"childIndex\" type=\"i\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"DeselectSelectedChild\">\n"
                "      <arg direction=\"in\" name=\"selectedChildIndex\" type=\"i\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"IsChildSelected\">\n"
                "      <arg direction=\"in\" name=\"childIndex\" type=\"i\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"SelectAll\">\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"ClearSelection\">\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"DeselectChild\">\n"
                "      <arg direction=\"in\" name=\"childIndex\" type=\"i\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                );

    static const QLatin1StringView tableIntrospection(
                "  <interface name=\"org.a11y.atspi.Table\">\n"
                "    <property access=\"read\" type=\"i\" name=\"NRows\"/>\n"
                "    <property access=\"read\" type=\"i\" name=\"NColumns\"/>\n"
                "    <property access=\"read\" type=\"(so)\" name=\"Caption\">\n"
                "      <annotation value=\"QSpiObjectReference\" name=\"org.qtproject.QtDBus.QtTypeName\"/>\n"
                "    </property>\n"
                "    <property access=\"read\" type=\"(so)\" name=\"Summary\">\n"
                "      <annotation value=\"QSpiObjectReference\" name=\"org.qtproject.QtDBus.QtTypeName\"/>\n"
                "    </property>\n"
                "    <property access=\"read\" type=\"i\" name=\"NSelectedRows\"/>\n"
                "    <property access=\"read\" type=\"i\" name=\"NSelectedColumns\"/>\n"
                "    <method name=\"GetAccessibleAt\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"column\"/>\n"
                "      <arg direction=\"out\" type=\"(so)\"/>\n"
                "      <annotation value=\"QSpiObjectReference\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetIndexAt\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"column\"/>\n"
                "      <arg direction=\"out\" type=\"i\"/>\n"
                "    </method>\n"
                "    <method name=\"GetRowAtIndex\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"index\"/>\n"
                "      <arg direction=\"out\" type=\"i\"/>\n"
                "    </method>\n"
                "    <method name=\"GetColumnAtIndex\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"index\"/>\n"
                "      <arg direction=\"out\" type=\"i\"/>\n"
                "    </method>\n"
                "    <method name=\"GetRowDescription\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "    </method>\n"
                "    <method name=\"GetColumnDescription\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"column\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "    </method>\n"
                "    <method name=\"GetRowExtentAt\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"column\"/>\n"
                "      <arg direction=\"out\" type=\"i\"/>\n"
                "    </method>\n"
                "    <method name=\"GetColumnExtentAt\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"column\"/>\n"
                "      <arg direction=\"out\" type=\"i\"/>\n"
                "    </method>\n"
                "    <method name=\"GetRowHeader\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"out\" type=\"(so)\"/>\n"
                "      <annotation value=\"QSpiObjectReference\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetColumnHeader\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"column\"/>\n"
                "      <arg direction=\"out\" type=\"(so)\"/>\n"
                "      <annotation value=\"QSpiObjectReference\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetSelectedRows\">\n"
                "      <arg direction=\"out\" type=\"ai\"/>\n"
                "      <annotation value=\"QSpiIntList\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetSelectedColumns\">\n"
                "      <arg direction=\"out\" type=\"ai\"/>\n"
                "      <annotation value=\"QSpiIntList\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"IsRowSelected\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"IsColumnSelected\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"column\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"IsSelected\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"column\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"AddRowSelection\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"AddColumnSelection\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"column\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"RemoveRowSelection\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"RemoveColumnSelection\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"column\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"GetRowColumnExtentsAtIndex\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"index\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"col\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"row_extents\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"col_extents\"/>\n"
                "      <arg direction=\"out\" type=\"b\" name=\"is_selected\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                );

    static const QLatin1StringView tableCellIntrospection(
                "  <interface name=\"org.a11y.atspi.TableCell\">\n"
                "    <property access=\"read\" name=\"ColumnSpan\" type=\"i\" />\n"
                "    <property access=\"read\" name=\"Position\" type=\"(ii)\">\n"
                "      <annotation name=\"org.qtproject.QtDBus.QtTypeName\" value=\"QPoint\"/>\n"
                "    </property>\n"
                "    <property access=\"read\" name=\"RowSpan\" type=\"i\" />\n"
                "    <property access=\"read\" name=\"Table\" type=\"(so)\" >\n"
                "      <annotation name=\"org.qtproject.QtDBus.QtTypeName\" value=\"QSpiObjectReference\"/>\n"
                "    </property>\n"
                "    <method name=\"GetRowColumnSpan\">\n"
                "      <arg direction=\"out\" type=\"b\" />\n"
                "      <arg direction=\"out\" name=\"row\" type=\"i\" />\n"
                "      <arg direction=\"out\" name=\"col\" type=\"i\" />\n"
                "      <arg direction=\"out\" name=\"row_extents\" type=\"i\" />\n"
                "      <arg direction=\"out\" name=\"col_extents\" type=\"i\" />\n"
                "    </method>\n"
                "    <method name=\"GetColumnHeaderCells\">\n"
                "      <arg direction=\"out\" type=\"a(so)\"/>\n"
                "      <annotation value=\"QSpiObjectReferenceArray\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetRowHeaderCells\">\n"
                "      <arg direction=\"out\" type=\"a(so)\"/>\n"
                "      <annotation value=\"QSpiObjectReferenceArray\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                );

    static const QLatin1StringView textIntrospection(
                "  <interface name=\"org.a11y.atspi.Text\">\n"
                "    <property access=\"read\" type=\"i\" name=\"CharacterCount\"/>\n"
                "    <property access=\"read\" type=\"i\" name=\"CaretOffset\"/>\n"
                "    <method name=\"GetStringAtOffset\">\n"
                "      <arg direction=\"in\" name=\"offset\" type=\"i\"/>\n"
                "      <arg direction=\"in\" name=\"granularity\" type=\"u\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "      <arg direction=\"out\" name=\"startOffset\" type=\"i\"/>\n"
                "      <arg direction=\"out\" name=\"endOffset\" type=\"i\"/>\n"
                "    </method>\n"
                "    <method name=\"GetText\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"startOffset\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"endOffset\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "    </method>\n"
                "    <method name=\"SetCaretOffset\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"offset\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"GetTextBeforeOffset\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"offset\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"type\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"startOffset\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"endOffset\"/>\n"
                "    </method>\n"
                "    <method name=\"GetTextAtOffset\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"offset\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"type\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"startOffset\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"endOffset\"/>\n"
                "    </method>\n"
                "    <method name=\"GetTextAfterOffset\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"offset\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"type\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"startOffset\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"endOffset\"/>\n"
                "    </method>\n"
                "    <method name=\"GetCharacterAtOffset\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"offset\"/>\n"
                "      <arg direction=\"out\" type=\"i\"/>\n"
                "    </method>\n"
                "    <method name=\"GetAttributeValue\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"offset\"/>\n"
                "      <arg direction=\"in\" type=\"s\" name=\"attributeName\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "    </method>\n"
                "    <method name=\"GetAttributes\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"offset\"/>\n"
                "      <arg direction=\"out\" type=\"a{ss}\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"startOffset\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"endOffset\"/>\n"
                "      <annotation value=\"QSpiAttributeSet\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetDefaultAttributes\">\n"
                "      <arg direction=\"out\" type=\"a{ss}\"/>\n"
                "      <annotation value=\"QSpiAttributeSet\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetCharacterExtents\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"offset\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"x\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"y\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"width\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"height\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"coordType\"/>\n"
                "    </method>\n"
                "    <method name=\"GetOffsetAtPoint\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"x\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"y\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"coordType\"/>\n"
                "      <arg direction=\"out\" type=\"i\"/>\n"
                "    </method>\n"
                "    <method name=\"GetNSelections\">\n"
                "      <arg direction=\"out\" type=\"i\"/>\n"
                "    </method>\n"
                "    <method name=\"GetSelection\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"selectionNum\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"startOffset\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"endOffset\"/>\n"
                "    </method>\n"
                "    <method name=\"AddSelection\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"startOffset\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"endOffset\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"RemoveSelection\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"selectionNum\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"SetSelection\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"selectionNum\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"startOffset\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"endOffset\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"GetRangeExtents\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"startOffset\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"endOffset\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"x\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"y\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"width\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"height\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"coordType\"/>\n"
                "    </method>\n"
                "    <method name=\"GetBoundedRanges\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"x\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"y\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"width\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"height\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"coordType\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"xClipType\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"yClipType\"/>\n"
                "      <arg direction=\"out\" type=\"a(iisv)\"/>\n"
                "      <annotation value=\"QSpiRangeList\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetAttributeRun\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"offset\"/>\n"
                "      <arg direction=\"in\" type=\"b\" name=\"includeDefaults\"/>\n"
                "      <arg direction=\"out\" type=\"a{ss}\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"startOffset\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"endOffset\"/>\n"
                "      <annotation value=\"QSpiAttributeSet\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetDefaultAttributeSet\">\n"
                "      <arg direction=\"out\" type=\"a{ss}\"/>\n"
                "      <annotation value=\"QSpiAttributeSet\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"ScrollSubstringTo\">\n"
                "      <arg direction=\"in\" name=\"startOffset\" type=\"i\"/>\n"
                "      <arg direction=\"in\" name=\"endOffset\" type=\"i\"/>\n"
                "      <arg direction=\"in\" name=\"type\" type=\"u\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                );

    static const QLatin1StringView valueIntrospection(
                "  <interface name=\"org.a11y.atspi.Value\">\n"
                "    <property access=\"read\" type=\"d\" name=\"MinimumValue\"/>\n"
                "    <property access=\"read\" type=\"d\" name=\"MaximumValue\"/>\n"
                "    <property access=\"read\" type=\"d\" name=\"MinimumIncrement\"/>\n"
                "    <property access=\"readwrite\" type=\"d\" name=\"CurrentValue\"/>\n"
                "    <method name=\"SetCurrentValue\">\n"
                "      <arg direction=\"in\" type=\"d\" name=\"value\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                );

    QAccessibleInterface * interface = interfaceFromPath(path);
    if (!interface) {
        qCWarning(lcAccessibilityAtspi) << "Could not find accessible on path:" << path;
        return QString();
    }

    QStringList interfaces = accessibleInterfaces(interface);

    QString xml;
    xml.append(accessibleIntrospection);

    if (interfaces.contains(ATSPI_DBUS_INTERFACE_COMPONENT ""_L1))
        xml.append(componentIntrospection);
    if (interfaces.contains(ATSPI_DBUS_INTERFACE_TEXT ""_L1))
        xml.append(textIntrospection);
    if (interfaces.contains(ATSPI_DBUS_INTERFACE_EDITABLE_TEXT ""_L1))
        xml.append(editableTextIntrospection);
    if (interfaces.contains(ATSPI_DBUS_INTERFACE_ACTION ""_L1))
        xml.append(actionIntrospection);
    if (interfaces.contains(ATSPI_DBUS_INTERFACE_SELECTION ""_L1))
        xml.append(selectionIntrospection);
    if (interfaces.contains(ATSPI_DBUS_INTERFACE_TABLE ""_L1))
        xml.append(tableIntrospection);
    if (interfaces.contains(ATSPI_DBUS_INTERFACE_TABLE_CELL ""_L1))
        xml.append(tableCellIntrospection);
    if (interfaces.contains(ATSPI_DBUS_INTERFACE_VALUE ""_L1))
        xml.append(valueIntrospection);
    if (path == ATSPI_DBUS_PATH_ROOT ""_L1)
        xml.append(applicationIntrospection);

    return xml;
}

void AtSpiAdaptor::setBitFlag(const QString &flag)
{
    Q_ASSERT(flag.size());

    // assume we don't get nonsense - look at first letter only
    switch (flag.at(0).toLower().toLatin1()) {
    case 'o': {
        if (flag.size() <= 8) { // Object::
            sendObject = 1;
        } else { // Object:Foo:Bar
            QString right = flag.mid(7);
            if (false) {
            } else if (right.startsWith("ActiveDescendantChanged"_L1)) {
                sendObject_active_descendant_changed = 1;
            } else if (right.startsWith("Announcement"_L1)) {
                sendObject_announcement = 1;
            } else if (right.startsWith("AttributesChanged"_L1)) {
                sendObject_attributes_changed = 1;
            } else if (right.startsWith("BoundsChanged"_L1)) {
                sendObject_bounds_changed = 1;
            } else if (right.startsWith("ChildrenChanged"_L1)) {
                sendObject_children_changed = 1;
            } else if (right.startsWith("ColumnDeleted"_L1)) {
                sendObject_column_deleted = 1;
            } else if (right.startsWith("ColumnInserted"_L1)) {
                sendObject_column_inserted = 1;
            } else if (right.startsWith("ColumnReordered"_L1)) {
                sendObject_column_reordered = 1;
            } else if (right.startsWith("LinkSelected"_L1)) {
                sendObject_link_selected = 1;
            } else if (right.startsWith("ModelChanged"_L1)) {
                sendObject_model_changed = 1;
            } else if (right.startsWith("PropertyChange"_L1)) {
                if (right == "PropertyChange:AccessibleDescription"_L1) {
                    sendObject_property_change_accessible_description = 1;
                } else if (right == "PropertyChange:AccessibleName"_L1) {
                    sendObject_property_change_accessible_name = 1;
                } else if (right == "PropertyChange:AccessibleParent"_L1) {
                    sendObject_property_change_accessible_parent = 1;
                } else if (right == "PropertyChange:AccessibleRole"_L1) {
                    sendObject_property_change_accessible_role = 1;
                } else if (right == "PropertyChange:TableCaption"_L1) {
                    sendObject_property_change_accessible_table_caption = 1;
                } else if (right == "PropertyChange:TableColumnDescription"_L1) {
                    sendObject_property_change_accessible_table_column_description = 1;
                } else if (right == "PropertyChange:TableColumnHeader"_L1) {
                    sendObject_property_change_accessible_table_column_header = 1;
                } else if (right == "PropertyChange:TableRowDescription"_L1) {
                    sendObject_property_change_accessible_table_row_description = 1;
                } else if (right == "PropertyChange:TableRowHeader"_L1) {
                    sendObject_property_change_accessible_table_row_header = 1;
                } else if (right == "PropertyChange:TableSummary"_L1) {
                    sendObject_property_change_accessible_table_summary = 1;
                } else if (right == "PropertyChange:AccessibleValue"_L1) {
                    sendObject_property_change_accessible_value = 1;
                } else {
                    sendObject_property_change = 1;
                }
            } else if (right.startsWith("RowDeleted"_L1)) {
                sendObject_row_deleted = 1;
            } else if (right.startsWith("RowInserted"_L1)) {
                sendObject_row_inserted = 1;
            } else if (right.startsWith("RowReordered"_L1)) {
                sendObject_row_reordered = 1;
            } else if (right.startsWith("SelectionChanged"_L1)) {
                sendObject_selection_changed = 1;
            } else if (right.startsWith("StateChanged"_L1)) {
                sendObject_state_changed = 1;
            } else if (right.startsWith("TextAttributesChanged"_L1)) {
                sendObject_text_attributes_changed = 1;
            } else if (right.startsWith("TextBoundsChanged"_L1)) {
                sendObject_text_bounds_changed = 1;
            } else if (right.startsWith("TextCaretMoved"_L1)) {
                sendObject_text_caret_moved = 1;
            } else if (right.startsWith("TextChanged"_L1)) {
                sendObject_text_changed = 1;
            } else if (right.startsWith("TextSelectionChanged"_L1)) {
                sendObject_text_selection_changed = 1;
            } else if (right.startsWith("ValueChanged"_L1)) {
                sendObject_value_changed = 1;
            } else if (right.startsWith("VisibleDataChanged"_L1)
                    || right.startsWith("VisibledataChanged"_L1)) { // typo in libatspi
                sendObject_visible_data_changed = 1;
            } else {
                qCWarning(lcAccessibilityAtspi) << "Subscription string not handled:" << flag;
            }
        }
        break;
    }
    case 'w': { // window
        if (flag.size() <= 8) {
            sendWindow = 1;
        } else { // object:Foo:Bar
            QString right = flag.mid(7);
            if (false) {
            } else if (right.startsWith("Activate"_L1)) {
                sendWindow_activate = 1;
            } else if (right.startsWith("Close"_L1)) {
                sendWindow_close= 1;
            } else if (right.startsWith("Create"_L1)) {
                sendWindow_create = 1;
            } else if (right.startsWith("Deactivate"_L1)) {
                sendWindow_deactivate = 1;
            } else if (right.startsWith("Lower"_L1)) {
                sendWindow_lower = 1;
            } else if (right.startsWith("Maximize"_L1)) {
                sendWindow_maximize = 1;
            } else if (right.startsWith("Minimize"_L1)) {
                sendWindow_minimize = 1;
            } else if (right.startsWith("Move"_L1)) {
                sendWindow_move = 1;
            } else if (right.startsWith("Raise"_L1)) {
                sendWindow_raise = 1;
            } else if (right.startsWith("Reparent"_L1)) {
                sendWindow_reparent = 1;
            } else if (right.startsWith("Resize"_L1)) {
                sendWindow_resize = 1;
            } else if (right.startsWith("Restore"_L1)) {
                sendWindow_restore = 1;
            } else if (right.startsWith("Restyle"_L1)) {
                sendWindow_restyle = 1;
            } else if (right.startsWith("Shade"_L1)) {
                sendWindow_shade = 1;
            } else if (right.startsWith("Unshade"_L1)) {
                sendWindow_unshade = 1;
            } else if (right.startsWith("DesktopCreate"_L1)) {
                // ignore this one
            } else if (right.startsWith("DesktopDestroy"_L1)) {
                // ignore this one
            } else {
                qCWarning(lcAccessibilityAtspi) << "Subscription string not handled:" << flag;
            }
        }
        break;
    }
    case 'f': {
        sendFocus = 1;
        break;
    }
    case 'd': { // document is not implemented
        break;
    }
    case 't': { // terminal is not implemented
        break;
    }
    case 'm': { // mouse* is handled in a different way by the gnome atspi stack
        break;
    }
    default:
        qCWarning(lcAccessibilityAtspi) << "Subscription string not handled:" << flag;
    }
}

/*!
  Checks via dbus which events should be sent.
  */
void AtSpiAdaptor::updateEventListeners()
{
    QDBusMessage m = QDBusMessage::createMethodCall("org.a11y.atspi.Registry"_L1,
                                                    "/org/a11y/atspi/registry"_L1,
                                                    "org.a11y.atspi.Registry"_L1, "GetRegisteredEvents"_L1);
    QDBusReply<QSpiEventListenerArray> listenersReply = m_dbus->connection().call(m);
    if (listenersReply.isValid()) {
        const QSpiEventListenerArray evList = listenersReply.value();
        for (const QSpiEventListener &ev : evList)
            setBitFlag(ev.eventName);
        m_applicationAdaptor->sendEvents(!evList.isEmpty());
    } else {
        qCDebug(lcAccessibilityAtspi) << "Could not query active accessibility event listeners.";
    }
}

void AtSpiAdaptor::eventListenerDeregistered(const QString &/*bus*/, const QString &/*path*/)
{
//    qCDebug(lcAccessibilityAtspi) << "AtSpiAdaptor::eventListenerDeregistered: " << bus << path;
    updateEventListeners();
}

void AtSpiAdaptor::eventListenerRegistered(const QString &/*bus*/, const QString &/*path*/)
{
//    qCDebug(lcAccessibilityAtspi) << "AtSpiAdaptor::eventListenerRegistered: " << bus << path;
    updateEventListeners();
}

/*!
  This slot needs to get called when a \a window has be activated or deactivated (become focused).
  When \a active is true, the window just received focus, otherwise it lost the focus.
  */
void AtSpiAdaptor::windowActivated(QObject* window, bool active)
{
    if (!(sendWindow || sendWindow_activate))
        return;

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(window);
    // If the window has been quickly activated or disabled, it will cause a crash.
    if (iface == nullptr)
        return;
    Q_ASSERT(!active || iface->isValid());

    QString windowTitle;
    // in dtor it may be invalid
    if (iface->isValid())
        windowTitle = iface->text(QAccessible::Name);

    QDBusVariant data;
    data.setVariant(windowTitle);

    QVariantList args = packDBusSignalArguments(QString(), 0, 0, QVariant::fromValue(data));

    QString status = active ? "Activate"_L1 : "Deactivate"_L1;
    QString path = pathForObject(window);
    sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_WINDOW ""_L1, status, args);

    QVariantList stateArgs = packDBusSignalArguments("active"_L1, active ? 1 : 0, 0, variantForPath(path));
    sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1, "StateChanged"_L1, stateArgs);
}

QVariantList AtSpiAdaptor::packDBusSignalArguments(const QString &type, int data1, int data2, const QVariant &variantData)
{
    QVariantList arguments;
    arguments << type << data1 << data2 << variantData << QMap<QString, QVariant>();
    return arguments;
}

QVariant AtSpiAdaptor::variantForPath(const QString &path) const
{
    QDBusVariant data;
    data.setVariant(QVariant::fromValue(QSpiObjectReference(m_dbus->connection(), QDBusObjectPath(path))));
    return QVariant::fromValue(data);
}

bool AtSpiAdaptor::sendDBusSignal(const QString &path, const QString &interface, const QString &signalName, const QVariantList &arguments) const
{
    QDBusMessage message = QDBusMessage::createSignal(path, interface, signalName);
    message.setArguments(arguments);
    return m_dbus->connection().send(message);
}

QAccessibleInterface *AtSpiAdaptor::interfaceFromPath(const QString& dbusPath) const
{
    if (dbusPath == ATSPI_DBUS_PATH_ROOT ""_L1)
        return QAccessible::queryAccessibleInterface(qApp);

    QStringList parts = dbusPath.split(u'/');
    if (parts.size() != 6) {
        qCDebug(lcAccessibilityAtspi) << "invalid path: " << dbusPath;
        return nullptr;
    }

    QString objectString = parts.at(5);
    QAccessible::Id id = objectString.toUInt();

    // The id is always in the range [INT_MAX+1, UINT_MAX]
    if ((int)id >= 0)
        qCWarning(lcAccessibilityAtspi) << "No accessible object found for id: " << id;

    return QAccessible::accessibleInterface(id);
}

void AtSpiAdaptor::notifyStateChange(QAccessibleInterface *interface, const QString &state, int value)
{
    QString path = pathForInterface(interface);
    QVariantList stateArgs = packDBusSignalArguments(state, value, 0, variantForPath(path));
    sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1, "StateChanged"_L1, stateArgs);
}

void AtSpiAdaptor::sendAnnouncement(QAccessibleAnnouncementEvent *event)
{
    QAccessibleInterface *iface = event->accessibleInterface();
    if (!iface) {
        qCWarning(lcAccessibilityAtspi, "Announcement event has no accessible set.");
        return;
    }
    if (!iface->isValid()) {
        qCWarning(lcAccessibilityAtspi) << "Announcement event with invalid accessible: " << iface;
        return;
    }

    const QString path = pathForInterface(iface);
    const QString message = event->message();
    const QAccessible::AnnouncementPoliteness prio = event->politeness();
    const int politeness = (prio == QAccessible::AnnouncementPoliteness::Assertive) ? ATSPI_LIVE_ASSERTIVE : ATSPI_LIVE_POLITE;

    const QVariantList args = packDBusSignalArguments(QString(), politeness, 0, QVariant::fromValue(QDBusVariant(message)));
    sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1, "Announcement"_L1, args);
}

/*!
    This function gets called when Qt notifies about accessibility updates.
*/
void AtSpiAdaptor::notify(QAccessibleEvent *event)
{
    switch (event->type()) {
    case QAccessible::ObjectCreated:
        if (sendObject || sendObject_children_changed)
            notifyAboutCreation(event->accessibleInterface());
        break;
    case QAccessible::ObjectShow: {
        if (sendObject || sendObject_state_changed) {
            notifyStateChange(event->accessibleInterface(), "showing"_L1, 1);
        }
        break;
    }
    case QAccessible::ObjectHide: {
        if (sendObject || sendObject_state_changed) {
            notifyStateChange(event->accessibleInterface(), "showing"_L1, 0);
        }
        break;
    }
    case QAccessible::ObjectDestroyed: {
        if (sendObject || sendObject_state_changed)
            notifyAboutDestruction(event->accessibleInterface());
        break;
    }
    case QAccessible::ObjectReorder: {
        if (sendObject || sendObject_children_changed)
            childrenChanged(event->accessibleInterface());
        break;
    }
    case QAccessible::NameChanged: {
        if (sendObject || sendObject_property_change || sendObject_property_change_accessible_name) {
            QAccessibleInterface *iface = event->accessibleInterface();
            if (!iface) {
                qCDebug(lcAccessibilityAtspi,
                        "NameChanged event from invalid accessible.");
                return;
            }

            QString path = pathForInterface(iface);
            QVariantList args = packDBusSignalArguments(
                "accessible-name"_L1, 0, 0,
                QVariant::fromValue(QDBusVariant(iface->text(QAccessible::Name))));
            sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1,
                           "PropertyChange"_L1, args);
        }
        break;
    }
    case QAccessible::DescriptionChanged: {
        if (sendObject || sendObject_property_change || sendObject_property_change_accessible_description) {
            QAccessibleInterface *iface = event->accessibleInterface();
            if (!iface) {
                qCDebug(lcAccessibilityAtspi,
                        "DescriptionChanged event from invalid accessible.");
                return;
            }

            QString path = pathForInterface(iface);
            QVariantList args = packDBusSignalArguments(
                "accessible-description"_L1, 0, 0,
                QVariant::fromValue(QDBusVariant(iface->text(QAccessible::Description))));
            sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1,
                           "PropertyChange"_L1, args);
        }
        break;
    }
    case QAccessible::Focus: {
        if (sendFocus || sendObject || sendObject_state_changed)
            sendFocusChanged(event->accessibleInterface());
        break;
    }

    case QAccessible::Announcement: {
        if (sendObject || sendObject_announcement) {
            QAccessibleAnnouncementEvent *announcementEvent = static_cast<QAccessibleAnnouncementEvent*>(event);
            sendAnnouncement(announcementEvent);
        }
        break;
    }
    case QAccessible::TextInserted:
    case QAccessible::TextRemoved:
    case QAccessible::TextUpdated: {
        if (sendObject || sendObject_text_changed) {
            QAccessibleInterface * iface = event->accessibleInterface();
            if (!iface || !iface->textInterface()) {
                qCWarning(lcAccessibilityAtspi) << "Received text event for invalid interface.";
                return;
            }
            QString path = pathForInterface(iface);

            int changePosition = 0;
            int cursorPosition = 0;
            QString textRemoved;
            QString textInserted;

            if (event->type() == QAccessible::TextInserted) {
                QAccessibleTextInsertEvent *textEvent = static_cast<QAccessibleTextInsertEvent*>(event);
                textInserted = textEvent->textInserted();
                changePosition = textEvent->changePosition();
                cursorPosition = textEvent->cursorPosition();
            } else if (event->type() == QAccessible::TextRemoved) {
                QAccessibleTextRemoveEvent *textEvent = static_cast<QAccessibleTextRemoveEvent*>(event);
                textRemoved = textEvent->textRemoved();
                changePosition = textEvent->changePosition();
                cursorPosition = textEvent->cursorPosition();
            } else if (event->type() == QAccessible::TextUpdated) {
                QAccessibleTextUpdateEvent *textEvent = static_cast<QAccessibleTextUpdateEvent*>(event);
                textInserted = textEvent->textInserted();
                textRemoved = textEvent->textRemoved();
                changePosition = textEvent->changePosition();
                cursorPosition = textEvent->cursorPosition();
            }

            QDBusVariant data;

            if (!textRemoved.isEmpty()) {
                data.setVariant(QVariant::fromValue(textRemoved));
                QVariantList args = packDBusSignalArguments("delete"_L1, changePosition, textRemoved.size(), QVariant::fromValue(data));
                sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1,
                               "TextChanged"_L1, args);
            }

            if (!textInserted.isEmpty()) {
                data.setVariant(QVariant::fromValue(textInserted));
                QVariantList args = packDBusSignalArguments("insert"_L1, changePosition, textInserted.size(), QVariant::fromValue(data));
                sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1,
                               "TextChanged"_L1, args);
            }

            // send a cursor update
            Q_UNUSED(cursorPosition);
//            QDBusVariant cursorData;
//            cursorData.setVariant(QVariant::fromValue(cursorPosition));
//            QVariantList args = packDBusSignalArguments(QString(), cursorPosition, 0, QVariant::fromValue(cursorData));
//            sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1,
//                           "TextCaretMoved"_L1, args);
        }
        break;
    }
    case QAccessible::TextCaretMoved: {
        if (sendObject || sendObject_text_caret_moved) {
            QAccessibleInterface * iface = event->accessibleInterface();
            if (!iface || !iface->textInterface()) {
                qCWarning(lcAccessibilityAtspi) << "Sending TextCaretMoved from object that does not implement text interface: " << iface;
                return;
            }

            QString path = pathForInterface(iface);
            QDBusVariant cursorData;
            int pos = iface->textInterface()->cursorPosition();
            cursorData.setVariant(QVariant::fromValue(pos));
            QVariantList args = packDBusSignalArguments(QString(), pos, 0, QVariant::fromValue(cursorData));
            sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1,
                           "TextCaretMoved"_L1, args);
        }
        break;
    }
    case QAccessible::TextSelectionChanged: {
        if (sendObject || sendObject_text_selection_changed) {
            QAccessibleInterface * iface = event->accessibleInterface();
            QString path = pathForInterface(iface);
            QVariantList args = packDBusSignalArguments(QString(), 0, 0, QVariant::fromValue(QDBusVariant(QVariant(QString()))));
            sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1,
                           "TextSelectionChanged"_L1, args);
        }
        break;
    }
    case QAccessible::ValueChanged: {
        if (sendObject || sendObject_value_changed || sendObject_property_change_accessible_value) {
            QAccessibleInterface * iface = event->accessibleInterface();
            if (!iface) {
                qCWarning(lcAccessibilityAtspi) << "ValueChanged event from invalid accessible.";
                return;
            }
            if (iface->valueInterface()) {
                QString path = pathForInterface(iface);
                QVariantList args = packDBusSignalArguments("accessible-value"_L1, 0, 0, variantForPath(path));
                sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1,
                               "PropertyChange"_L1, args);
            } else if (iface->role() == QAccessible::ComboBox) {
                // Combo Box with AT-SPI likes to be special
                // It requires a name-change to update caches and then selection-changed
                QString path = pathForInterface(iface);
                QVariantList args1 = packDBusSignalArguments(
                    "accessible-name"_L1, 0, 0,
                    QVariant::fromValue(QDBusVariant(iface->text(QAccessible::Name))));
                sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1,
                               "PropertyChange"_L1, args1);
                QVariantList args2 = packDBusSignalArguments(QString(), 0, 0, QVariant::fromValue(QDBusVariant(QVariant(0))));
                sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1,
                               "SelectionChanged"_L1, args2);
            } else {
                qCWarning(lcAccessibilityAtspi) << "ValueChanged event and no ValueInterface or ComboBox: " << iface;
            }
        }
        break;
    }
    case QAccessible::SelectionAdd:
    case QAccessible::SelectionRemove:
    case QAccessible::Selection: {
        QAccessibleInterface * iface = event->accessibleInterface();
        if (!iface) {
            qCWarning(lcAccessibilityAtspi) << "Selection event from invalid accessible.";
            return;
        }
        // send event for change of selected state for the interface itself
        QString path = pathForInterface(iface);
        int selected = iface->state().selected ? 1 : 0;
        QVariantList stateArgs = packDBusSignalArguments("selected"_L1, selected, 0, variantForPath(path));
        sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1, "StateChanged"_L1, stateArgs);

        // send SelectionChanged event for the parent
        QAccessibleInterface* parent = iface->parent();
        if (!parent) {
            qCDebug(lcAccessibilityAtspi) << "No valid parent in selection event.";
            return;
        }

        QString parentPath = pathForInterface(parent);
        QVariantList args = packDBusSignalArguments(QString(), 0, 0, variantForPath(parentPath));
        sendDBusSignal(parentPath, QLatin1String(ATSPI_DBUS_INTERFACE_EVENT_OBJECT),
                       QLatin1String("SelectionChanged"), args);
        break;
    }
    case QAccessible::SelectionWithin: {
        QAccessibleInterface * iface = event->accessibleInterface();
        if (!iface) {
            qCWarning(lcAccessibilityAtspi) << "SelectionWithin event from invalid accessible.";
            return;
        }

        QString path = pathForInterface(iface);
        QVariantList args = packDBusSignalArguments(QString(), 0, 0, variantForPath(path));
        sendDBusSignal(path, QLatin1String(ATSPI_DBUS_INTERFACE_EVENT_OBJECT), QLatin1String("SelectionChanged"), args);
        break;
    }
    case QAccessible::StateChanged: {
        if (sendObject || sendObject_state_changed || sendWindow || sendWindow_activate) {
            QAccessible::State stateChange = static_cast<QAccessibleStateChangeEvent*>(event)->changedStates();
            if (stateChange.checked) {
                QAccessibleInterface * iface = event->accessibleInterface();
                if (!iface) {
                    qCWarning(lcAccessibilityAtspi) << "StateChanged event from invalid accessible.";
                    return;
                }
                int checked = iface->state().checked;
                notifyStateChange(iface, "checked"_L1, checked);
            } else if (stateChange.active) {
                QAccessibleInterface * iface = event->accessibleInterface();
                if (!iface || !(iface->role() == QAccessible::Window && (sendWindow || sendWindow_activate)))
                    return;
                int isActive = iface->state().active;
                QString windowTitle = iface->text(QAccessible::Name);
                QDBusVariant data;
                data.setVariant(windowTitle);
                QVariantList args = packDBusSignalArguments(QString(), 0, 0, QVariant::fromValue(data));
                QString status = isActive ? "Activate"_L1 : "Deactivate"_L1;
                QString path = pathForInterface(iface);
                sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_WINDOW ""_L1, status, args);
                notifyStateChange(iface, "active"_L1, isActive);
            } else if (stateChange.disabled) {
                QAccessibleInterface *iface = event->accessibleInterface();
                QAccessible::State state = iface->state();
                bool enabled = !state.disabled;

                notifyStateChange(iface, "enabled"_L1, enabled);
                notifyStateChange(iface, "sensitive"_L1, enabled);
            } else if (stateChange.focused) {
                QAccessibleInterface *iface = event->accessibleInterface();
                QAccessible::State state = iface->state();
                bool focused = state.focused;
                notifyStateChange(iface, "focused"_L1, focused);
            }
        }
        break;
    }
    case QAccessible::TableModelChanged: {
        QAccessibleInterface *interface = event->accessibleInterface();
        if (!interface || !interface->isValid()) {
            qCWarning(lcAccessibilityAtspi) << "TableModelChanged event from invalid accessible.";
            return;
        }

        const QString path = pathForInterface(interface);
        QAccessibleTableModelChangeEvent *tableModelEvent = static_cast<QAccessibleTableModelChangeEvent*>(event);
        switch (tableModelEvent->modelChangeType()) {
        case QAccessibleTableModelChangeEvent::ColumnsInserted: {
            if (sendObject || sendObject_column_inserted) {
                const int firstColumn = tableModelEvent->firstColumn();
                const int insertedColumnCount = tableModelEvent->lastColumn() - firstColumn + 1;
                QVariantList args = packDBusSignalArguments(QString(), firstColumn, insertedColumnCount, QVariant::fromValue(QDBusVariant(QVariant(QString()))));
                sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1, "ColumnInserted"_L1, args);
            }
            break;
        }
        case QAccessibleTableModelChangeEvent::ColumnsRemoved: {
            if (sendObject || sendObject_column_deleted) {
                const int firstColumn = tableModelEvent->firstColumn();
                const int removedColumnCount = tableModelEvent->lastColumn() - firstColumn + 1;
                QVariantList args = packDBusSignalArguments(QString(), firstColumn, removedColumnCount, QVariant::fromValue(QDBusVariant(QVariant(QString()))));
                sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1, "ColumnDeleted"_L1, args);
            }
            break;
        }
        case QAccessibleTableModelChangeEvent::RowsInserted: {
            if (sendObject || sendObject_row_inserted) {
                const int firstRow = tableModelEvent->firstRow();
                const int insertedRowCount = tableModelEvent->lastRow() - firstRow + 1;
                QVariantList args = packDBusSignalArguments(QString(), firstRow, insertedRowCount, QVariant::fromValue(QDBusVariant(QVariant(QString()))));
                sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1, "RowInserted"_L1, args);
            }
            break;
        }
        case QAccessibleTableModelChangeEvent::RowsRemoved: {
            if (sendObject || sendObject_row_deleted) {
                const int firstRow = tableModelEvent->firstRow();
                const int removedRowCount = tableModelEvent->lastRow() - firstRow + 1;
                QVariantList args = packDBusSignalArguments(QString(), firstRow, removedRowCount, QVariant::fromValue(QDBusVariant(QVariant(QString()))));
                sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1, "RowDeleted"_L1, args);
            }
            break;
        }
        case QAccessibleTableModelChangeEvent::ModelChangeType::ModelReset: {
            if (sendObject || sendObject_model_changed) {
                QVariantList args = packDBusSignalArguments(QString(), 0, 0, QVariant::fromValue(QDBusVariant(QVariant(QString()))));
                sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1, "ModelChanged"_L1, args);
            }
            break;
        }
        case QAccessibleTableModelChangeEvent::DataChanged: {
            if (sendObject || sendObject_visible_data_changed) {
                QVariantList args = packDBusSignalArguments(QString(), 0, 0, QVariant::fromValue(QDBusVariant(QVariant(QString()))));
                sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1, "VisibleDataChanged"_L1, args);
            }
            break;
        }
        }
        break;
    }

        // For now we ignore these events
    case QAccessible::ParentChanged:
    case QAccessible::DialogStart:
    case QAccessible::DialogEnd:
    case QAccessible::PopupMenuStart:
    case QAccessible::PopupMenuEnd:
    case QAccessible::SoundPlayed:
    case QAccessible::Alert:
    case QAccessible::ForegroundChanged:
    case QAccessible::MenuStart:
    case QAccessible::MenuEnd:
    case QAccessible::ContextHelpStart:
    case QAccessible::ContextHelpEnd:
    case QAccessible::DragDropStart:
    case QAccessible::DragDropEnd:
    case QAccessible::ScrollingStart:
    case QAccessible::ScrollingEnd:
    case QAccessible::MenuCommand:
    case QAccessible::ActionChanged:
    case QAccessible::ActiveDescendantChanged:
    case QAccessible::AttributeChanged:
    case QAccessible::DocumentContentChanged:
    case QAccessible::DocumentLoadComplete:
    case QAccessible::DocumentLoadStopped:
    case QAccessible::DocumentReload:
    case QAccessible::HyperlinkEndIndexChanged:
    case QAccessible::HyperlinkNumberOfAnchorsChanged:
    case QAccessible::HyperlinkSelectedLinkChanged:
    case QAccessible::HypertextLinkActivated:
    case QAccessible::HypertextLinkSelected:
    case QAccessible::HyperlinkStartIndexChanged:
    case QAccessible::HypertextChanged:
    case QAccessible::HypertextNLinksChanged:
    case QAccessible::ObjectAttributeChanged:
    case QAccessible::PageChanged:
    case QAccessible::SectionChanged:
    case QAccessible::TableCaptionChanged:
    case QAccessible::TableColumnDescriptionChanged:
    case QAccessible::TableColumnHeaderChanged:
    case QAccessible::TableRowDescriptionChanged:
    case QAccessible::TableRowHeaderChanged:
    case QAccessible::TableSummaryChanged:
    case QAccessible::TextAttributeChanged:
    case QAccessible::TextColumnChanged:
    case QAccessible::VisibleDataChanged:
    case QAccessible::LocationChanged:
    case QAccessible::HelpChanged:
    case QAccessible::DefaultActionChanged:
    case QAccessible::AcceleratorChanged:
    case QAccessible::IdentifierChanged:
    case QAccessible::InvalidEvent:
        break;
    }
}

void AtSpiAdaptor::sendFocusChanged(QAccessibleInterface *interface) const
{
    static QString lastFocusPath;
    // "remove" old focus
    if (!lastFocusPath.isEmpty()) {
        QVariantList stateArgs = packDBusSignalArguments("focused"_L1, 0, 0, variantForPath(lastFocusPath));
        sendDBusSignal(lastFocusPath, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1,
                       "StateChanged"_L1, stateArgs);
    }
    // send new focus
    {
        QString path = pathForInterface(interface);

        QVariantList stateArgs = packDBusSignalArguments("focused"_L1, 1, 0, variantForPath(path));
        sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1,
                       "StateChanged"_L1, stateArgs);

        QVariantList focusArgs = packDBusSignalArguments(QString(), 0, 0, variantForPath(path));
        sendDBusSignal(path, ATSPI_DBUS_INTERFACE_EVENT_FOCUS ""_L1, "Focus"_L1, focusArgs);
        lastFocusPath = path;
    }
}

void AtSpiAdaptor::childrenChanged(QAccessibleInterface *interface) const
{
    QString parentPath = pathForInterface(interface);
    const int childCount = interface->childCount();
    for (int i = 0; i < childCount; ++i) {
        QString childPath = pathForInterface(interface->child(i));
        QVariantList args = packDBusSignalArguments("add"_L1, i, 0, childPath);
        sendDBusSignal(parentPath, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1, "ChildrenChanged"_L1, args);
    }
}

void AtSpiAdaptor::notifyAboutCreation(QAccessibleInterface *interface) const
{
    // notify about the new child of our parent
    QAccessibleInterface * parent = interface->parent();
    if (!parent) {
        qCDebug(lcAccessibilityAtspi) << "AtSpiAdaptor::notifyAboutCreation: Could not find parent for " << interface->object();
        return;
    }
    QString path = pathForInterface(interface);
    const int childIndex = parent->indexOfChild(interface);
    QString parentPath = pathForInterface(parent);
    QVariantList args = packDBusSignalArguments("add"_L1, childIndex, 0, variantForPath(path));
    sendDBusSignal(parentPath, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1, "ChildrenChanged"_L1, args);
}

void AtSpiAdaptor::notifyAboutDestruction(QAccessibleInterface *interface) const
{
    if (!interface || !interface->isValid())
        return;

    QAccessibleInterface * parent = interface->parent();
    if (!parent) {
        qCDebug(lcAccessibilityAtspi) << "AtSpiAdaptor::notifyAboutDestruction: Could not find parent for " << interface->object();
        return;
    }
    QString path = pathForInterface(interface);

    // this is in the destructor. we have no clue which child we used to be.
    // FIXME
    int childIndex = -1;
    //    if (child) {
    //        childIndex = child;
    //    } else {
    //        childIndex = parent->indexOfChild(interface);
    //    }

    QString parentPath = pathForInterface(parent);
    QVariantList args = packDBusSignalArguments("remove"_L1, childIndex, 0, variantForPath(path));
    sendDBusSignal(parentPath, ATSPI_DBUS_INTERFACE_EVENT_OBJECT ""_L1, "ChildrenChanged"_L1, args);
}

/*!
  Handle incoming DBus message.
  This function dispatches the dbus message to the right interface handler.
  */
bool AtSpiAdaptor::handleMessage(const QDBusMessage &message, const QDBusConnection &connection)
{
    // get accessible interface
    QAccessibleInterface * accessible = interfaceFromPath(message.path());
    if (!accessible) {
        qCWarning(lcAccessibilityAtspi) << "Could not find accessible on path:" << message.path();
        return false;
    }
    if (!accessible->isValid()) {
        qCWarning(lcAccessibilityAtspi) << "Accessible invalid:" << accessible << message.path();
        return false;
    }

    QString interface = message.interface();
    QString function = message.member();

    // qCDebug(lcAccessibilityAtspi) << "AtSpiAdaptor::handleMessage: " << interface << function;

    if (function == "Introspect"_L1) {
        //introspect(message.path());
        return false;
    }

    // handle properties like regular functions
    if (interface == "org.freedesktop.DBus.Properties"_L1) {
        const auto arguments = message.arguments();
        if (arguments.size() > 0) {
            interface = arguments.at(0).toString();
            if (arguments.size() > 1) // e.g. Get/Set + Name
                function = function + arguments.at(1).toString();
        }
    }

    // switch interface to call
    if (interface == ATSPI_DBUS_INTERFACE_ACCESSIBLE ""_L1)
        return accessibleInterface(accessible, function, message, connection);
    if (interface == ATSPI_DBUS_INTERFACE_APPLICATION ""_L1)
        return applicationInterface(accessible, function, message, connection);
    if (interface == ATSPI_DBUS_INTERFACE_COMPONENT ""_L1)
        return componentInterface(accessible, function, message, connection);
    if (interface == ATSPI_DBUS_INTERFACE_ACTION ""_L1)
        return actionInterface(accessible, function, message, connection);
    if (interface == ATSPI_DBUS_INTERFACE_SELECTION ""_L1)
        return selectionInterface(accessible, function, message, connection);
    if (interface == ATSPI_DBUS_INTERFACE_TEXT ""_L1)
        return textInterface(accessible, function, message, connection);
    if (interface == ATSPI_DBUS_INTERFACE_EDITABLE_TEXT ""_L1)
        return editableTextInterface(accessible, function, message, connection);
    if (interface == ATSPI_DBUS_INTERFACE_VALUE ""_L1)
        return valueInterface(accessible, function, message, connection);
    if (interface == ATSPI_DBUS_INTERFACE_TABLE ""_L1)
        return tableInterface(accessible, function, message, connection);
    if (interface == ATSPI_DBUS_INTERFACE_TABLE_CELL ""_L1)
        return tableCellInterface(accessible, function, message, connection);

    qCDebug(lcAccessibilityAtspi) << "AtSpiAdaptor::handleMessage with unknown interface: " << message.path() << interface << function;
    return false;
}

// Application
bool AtSpiAdaptor::applicationInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection)
{
    if (message.path() != ATSPI_DBUS_PATH_ROOT ""_L1) {
        qCWarning(lcAccessibilityAtspi) << "Could not find application interface for:" << message.path() << interface;
        return false;
    }

    if (function == "SetId"_L1) {
        Q_ASSERT(message.signature() == "ssv"_L1);
        QVariant value = qvariant_cast<QDBusVariant>(message.arguments().at(2)).variant();

        m_applicationId = value.toInt();
        return true;
    }
    if (function == "GetId"_L1) {
        Q_ASSERT(message.signature() == "ss"_L1);
        QDBusMessage reply = message.createReply(QVariant::fromValue(QDBusVariant(m_applicationId)));
        return connection.send(reply);
    }
    if (function == "GetAtspiVersion"_L1) {
        Q_ASSERT(message.signature() == "ss"_L1);
        // return "2.1" as described in the Application interface spec
        QDBusMessage reply = message.createReply(QVariant::fromValue(QDBusVariant("2.1"_L1)));
        return connection.send(reply);
    }
    if (function == "GetToolkitName"_L1) {
        Q_ASSERT(message.signature() == "ss"_L1);
        QDBusMessage reply = message.createReply(QVariant::fromValue(QDBusVariant("Qt"_L1)));
        return connection.send(reply);
    }
    if (function == "GetVersion"_L1) {
        Q_ASSERT(message.signature() == "ss"_L1);
        QDBusMessage reply = message.createReply(QVariant::fromValue(QDBusVariant(QLatin1StringView(qVersion()))));
        return connection.send(reply);
    }
    if (function == "GetLocale"_L1) {
        Q_ASSERT(message.signature() == "u"_L1);
        QDBusMessage reply = message.createReply(QVariant::fromValue(QLocale().name()));
        return connection.send(reply);
    }
    qCWarning(lcAccessibilityAtspi) << "AtSpiAdaptor::applicationInterface does not implement"
                                    << function << message.path();
    return false;
}

/*!
  Register this application as accessible on the accessibility DBus.
  */
void AtSpiAdaptor::registerApplication()
{
    OrgA11yAtspiSocketInterface registry(ATSPI_DBUS_NAME_REGISTRY ""_L1, ATSPI_DBUS_PATH_ROOT ""_L1,
                                         m_dbus->connection());

    QDBusPendingReply<QSpiObjectReference> reply;
    QSpiObjectReference ref = QSpiObjectReference(m_dbus->connection(), QDBusObjectPath(ATSPI_DBUS_PATH_ROOT));
    reply = registry.Embed(ref);
    reply.waitForFinished(); // TODO: make this async
    if (reply.isValid ()) {
        const QSpiObjectReference &socket = reply.value();
        m_accessibilityRegistry = QSpiObjectReference(socket);
    } else {
        qCWarning(lcAccessibilityAtspi) << "Error in contacting registry:"
                   << reply.error().name()
                   << reply.error().message();
    }
}

// Accessible
bool AtSpiAdaptor::accessibleInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection)
{
    if (function == "GetRole"_L1) {
        sendReply(connection, message, (uint) getRole(interface));
    } else if (function == "GetName"_L1) {
        sendReply(connection, message, QVariant::fromValue(QDBusVariant(interface->text(QAccessible::Name))));
    } else if (function == "GetRoleName"_L1) {
        sendReply(connection, message, QSpiAccessibleBridge::namesForRole(interface->role()).name());
    } else if (function == "GetLocalizedRoleName"_L1) {
        sendReply(connection, message, QVariant::fromValue(QSpiAccessibleBridge::namesForRole(interface->role()).localizedName()));
    } else if (function == "GetChildCount"_L1) {
        sendReply(connection, message, QVariant::fromValue(QDBusVariant(interface->childCount())));
    } else if (function == "GetIndexInParent"_L1) {
        int childIndex = -1;
        QAccessibleInterface * parent = interface->parent();
        if (parent) {
            childIndex = parent->indexOfChild(interface);
            if (childIndex < 0) {
                qCDebug(lcAccessibilityAtspi) <<  "GetIndexInParent get invalid index: " << childIndex << interface;
            }
        }
        sendReply(connection, message, childIndex);
    } else if (function == "GetParent"_L1) {
        QString path;
        QAccessibleInterface * parent = interface->parent();
        if (!parent) {
            if (interface->object() == qApp) {
                sendReply(connection, message,
                          QVariant::fromValue(QDBusVariant(QVariant::fromValue(m_accessibilityRegistry))));
                return true;
            }

            path = ATSPI_DBUS_PATH_NULL ""_L1;
        } else if (parent->role() == QAccessible::Application) {
            path = ATSPI_DBUS_PATH_ROOT ""_L1;
        } else {
            path = pathForInterface(parent);
        }
        // Parent is a property, so it needs to be wrapped inside an extra variant.
        sendReply(connection, message, QVariant::fromValue(
                      QDBusVariant(QVariant::fromValue(QSpiObjectReference(connection, QDBusObjectPath(path))))));
    } else if (function == "GetChildAtIndex"_L1) {
        const int index = message.arguments().at(0).toInt();
        if (index < 0) {
            sendReply(connection, message, QVariant::fromValue(
                          QSpiObjectReference(connection, QDBusObjectPath(ATSPI_DBUS_PATH_NULL))));
        } else {
            QAccessibleInterface * childInterface = interface->child(index);
            sendReply(connection, message, QVariant::fromValue(
                          QSpiObjectReference(connection, QDBusObjectPath(pathForInterface(childInterface)))));
        }
    } else if (function == "GetInterfaces"_L1) {
        sendReply(connection, message, accessibleInterfaces(interface));
    } else if (function == "GetDescription"_L1) {
        sendReply(connection, message, QVariant::fromValue(QDBusVariant(interface->text(QAccessible::Description))));
    } else if (function == "GetHelpText"_L1) {
        sendReply(connection, message, QVariant::fromValue(QDBusVariant(interface->text(QAccessible::Help))));
    } else if (function == "GetState"_L1) {
        quint64 spiState = spiStatesFromQState(interface->state());
        if (interface->tableInterface()) {
            // For tables, setting manages_descendants should
            // indicate to the client that it cannot cache these
            // interfaces.
            setSpiStateBit(&spiState, ATSPI_STATE_MANAGES_DESCENDANTS);
        }
        QAccessible::Role role = interface->role();
        if (role == QAccessible::TreeItem ||
            role == QAccessible::ListItem) {
            /* Transient means libatspi2 will not cache items.
               This is important because when adding/removing an item
               the cache becomes outdated and we don't change the paths of
               items in lists/trees/tables. */
            setSpiStateBit(&spiState, ATSPI_STATE_TRANSIENT);
        }
        sendReply(connection, message,
                  QVariant::fromValue(spiStateSetFromSpiStates(spiState)));
    } else if (function == "GetAttributes"_L1) {
        sendReply(connection, message, QVariant::fromValue(getAttributes(interface)));
    } else if (function == "GetRelationSet"_L1) {
        sendReply(connection, message, QVariant::fromValue(relationSet(interface, connection)));
    } else if (function == "GetApplication"_L1) {
        sendReply(connection, message, QVariant::fromValue(
                      QSpiObjectReference(connection, QDBusObjectPath(ATSPI_DBUS_PATH_ROOT))));
    } else if (function == "GetLocale"_L1) {
        QLocale locale;
        if (QAccessibleAttributesInterface *attributesIface = interface->attributesInterface()) {
            const QVariant localeVariant = attributesIface->attributeValue(QAccessible::Attribute::Locale);
            if (localeVariant.isValid()) {
                Q_ASSERT(localeVariant.canConvert<QLocale>());
                locale = localeVariant.toLocale();
            }
        }
        sendReply(connection, message, QVariant::fromValue(QDBusVariant(locale.name())));
    } else if (function == "GetChildren"_L1) {
        QSpiObjectReferenceArray children;
        const int numChildren = interface->childCount();
        children.reserve(numChildren);
        for (int i = 0; i < numChildren; ++i) {
            QString childPath = pathForInterface(interface->child(i));
            QSpiObjectReference ref(connection, QDBusObjectPath(childPath));
            children << ref;
        }
        connection.send(message.createReply(QVariant::fromValue(children)));
    } else if (function == "GetAccessibleId"_L1) {
        sendReply(connection, message,
                  QVariant::fromValue(QDBusVariant(QAccessibleBridgeUtils::accessibleId(interface))));
    } else {
        qCWarning(lcAccessibilityAtspi) << "AtSpiAdaptor::accessibleInterface does not implement" << function << message.path();
        return false;
    }
    return true;
}

AtspiRole AtSpiAdaptor::getRole(QAccessibleInterface *interface) const
{
    if ((interface->role() == QAccessible::EditableText) && interface->state().passwordEdit)
        return ATSPI_ROLE_PASSWORD_TEXT;
    return QSpiAccessibleBridge::namesForRole(interface->role()).spiRole();
}

QStringList AtSpiAdaptor::accessibleInterfaces(QAccessibleInterface *interface) const
{
    QStringList ifaces;
    qCDebug(lcAccessibilityAtspiCreation) << "AtSpiAdaptor::accessibleInterfaces create: " << interface->object();
    ifaces << u"" ATSPI_DBUS_INTERFACE_ACCESSIBLE ""_s;

    if (    (!interface->rect().isEmpty()) ||
            (interface->object() && interface->object()->isWidgetType()) ||
            (interface->role() == QAccessible::ListItem) ||
            (interface->role() == QAccessible::Cell) ||
            (interface->role() == QAccessible::TreeItem) ||
            (interface->role() == QAccessible::Row) ||
            (interface->object() && interface->object()->inherits("QSGItem"))
            ) {
        ifaces << u"" ATSPI_DBUS_INTERFACE_COMPONENT ""_s;
    } else {
        qCDebug(lcAccessibilityAtspiCreation) << " IS NOT a component";
    }
    if (interface->role() == QAccessible::Application)
        ifaces << u"" ATSPI_DBUS_INTERFACE_APPLICATION ""_s;

    if (interface->actionInterface() || interface->valueInterface())
        ifaces << u"" ATSPI_DBUS_INTERFACE_ACTION ""_s;

    if (interface->selectionInterface())
        ifaces << ATSPI_DBUS_INTERFACE_SELECTION ""_L1;

    if (interface->textInterface())
        ifaces << u"" ATSPI_DBUS_INTERFACE_TEXT ""_s;

    if (interface->editableTextInterface())
        ifaces << u"" ATSPI_DBUS_INTERFACE_EDITABLE_TEXT ""_s;

    if (interface->valueInterface())
        ifaces << u"" ATSPI_DBUS_INTERFACE_VALUE ""_s;

    if (interface->tableInterface())
        ifaces <<  u"" ATSPI_DBUS_INTERFACE_TABLE ""_s;

    if (interface->tableCellInterface())
        ifaces <<  u"" ATSPI_DBUS_INTERFACE_TABLE_CELL ""_s;

    return ifaces;
}

QSpiRelationArray AtSpiAdaptor::relationSet(QAccessibleInterface *interface, const QDBusConnection &connection) const
{
    typedef std::pair<QAccessibleInterface*, QAccessible::Relation> RelationPair;
    const QList<RelationPair> relationInterfaces = interface->relations();

    QSpiRelationArray relations;
    for (const RelationPair &pair : relationInterfaces) {
// FIXME: this loop seems a bit strange... "related" always have one item when we check.
//And why is it a list, when it always have one item? And it seems to assume that the QAccessible::Relation enum maps directly to AtSpi
        QSpiObjectReferenceArray related;

        QDBusObjectPath path = QDBusObjectPath(pathForInterface(pair.first));
        related.append(QSpiObjectReference(connection, path));

        if (!related.isEmpty())
            relations.append(QSpiRelationArrayEntry(qAccessibleRelationToAtSpiRelation(pair.second), related));
    }
    return relations;
}

void AtSpiAdaptor::sendReply(const QDBusConnection &connection, const QDBusMessage &message, const QVariant &argument) const
{
    QDBusMessage reply = message.createReply(argument);
    connection.send(reply);
}


QString AtSpiAdaptor::pathForObject(QObject *object) const
{
    Q_ASSERT(object);

    if (inheritsQAction(object)) {
        qCWarning(lcAccessibilityAtspi) << "AtSpiAdaptor::pathForObject: Creating path with QAction as object.";
    }

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(object);
    return pathForInterface(iface);
}

QString AtSpiAdaptor::pathForInterface(QAccessibleInterface *interface) const
{
    if (!interface || !interface->isValid())
        return u"" ATSPI_DBUS_PATH_NULL ""_s;
    if (interface->role() == QAccessible::Application)
        return u"" ATSPI_DBUS_PATH_ROOT ""_s;

    QAccessible::Id id = QAccessible::uniqueId(interface);
    Q_ASSERT((int)id < 0);
    return QSPI_OBJECT_PATH_PREFIX ""_L1 + QString::number(id);
}

bool AtSpiAdaptor::inheritsQAction(QObject *object)
{
    const QMetaObject *mo = object->metaObject();
    while (mo) {
        const QLatin1StringView cn(mo->className());
        if (cn == "QAction"_L1)
            return true;
        mo = mo->superClass();
    }
    return false;
}

// Component
static QAccessibleInterface * getWindow(QAccessibleInterface * interface)
{
    // find top-level window in a11y hierarchy (either has a
    // corresponding role or is a direct child of the application object)
    QAccessibleInterface* app = QAccessible::queryAccessibleInterface(qApp);
    while (interface && interface->role() != QAccessible::Dialog
           && interface->role() != QAccessible::Window && interface->parent() != app)
        interface = interface->parent();

    return interface;
}

bool AtSpiAdaptor::componentInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection)
{
    if (function == "Contains"_L1) {
        bool ret = false;
        int x = message.arguments().at(0).toInt();
        int y = message.arguments().at(1).toInt();
        uint coordType = message.arguments().at(2).toUInt();
        if (!isValidCoordType(coordType))
            return false;
        ret = getExtents(interface, coordType).contains(x, y);
        sendReply(connection, message, ret);
    } else if (function == "GetAccessibleAtPoint"_L1) {
        QPoint point(message.arguments().at(0).toInt(), message.arguments().at(1).toInt());
        uint coordType = message.arguments().at(2).toUInt();
        if (!isValidCoordType(coordType))
            return false;
        QPoint screenPos = translateToScreenCoordinates(interface, point, coordType);

        QAccessibleInterface * childInterface(interface->childAt(screenPos.x(), screenPos.y()));
        QAccessibleInterface * iface = nullptr;
        while (childInterface) {
            iface = childInterface;
            childInterface = iface->childAt(screenPos.x(), screenPos.y());
        }
        if (iface) {
            QString path = pathForInterface(iface);
            sendReply(connection, message, QVariant::fromValue(
                          QSpiObjectReference(connection, QDBusObjectPath(path))));
        } else {
            sendReply(connection, message, QVariant::fromValue(
                          QSpiObjectReference(connection, QDBusObjectPath(ATSPI_DBUS_PATH_NULL))));
        }
    } else if (function == "GetAlpha"_L1) {
        sendReply(connection, message, (double) 1.0);
    } else if (function == "GetExtents"_L1) {
        uint coordType = message.arguments().at(0).toUInt();
        if (!isValidCoordType(coordType))
            return false;
        sendReply(connection, message, QVariant::fromValue(getExtents(interface, coordType)));
    } else if (function == "GetLayer"_L1) {
        sendReply(connection, message, QVariant::fromValue((uint)1));
    } else if (function == "GetMDIZOrder"_L1) {
        sendReply(connection, message, QVariant::fromValue((short)0));
    } else if (function == "GetPosition"_L1) {
        uint coordType = message.arguments().at(0).toUInt();
        if (!isValidCoordType(coordType))
            return false;
        QRect rect = getExtents(interface, coordType);
        QVariantList pos;
        pos << rect.x() << rect.y();
        connection.send(message.createReply(pos));
    } else if (function == "GetSize"_L1) {
        QRect rect = interface->rect();
        QVariantList size;
        size << rect.width() << rect.height();
        connection.send(message.createReply(size));
    } else if (function == "GrabFocus"_L1) {
        QAccessibleActionInterface *actionIface = interface->actionInterface();
        if (actionIface && actionIface->actionNames().contains(QAccessibleActionInterface::setFocusAction())) {
            actionIface->doAction(QAccessibleActionInterface::setFocusAction());
            sendReply(connection, message, true);
        } else {
            sendReply(connection, message, false);
        }
    } else if (function == "SetExtents"_L1) {
//        int x = message.arguments().at(0).toInt();
//        int y = message.arguments().at(1).toInt();
//        int width = message.arguments().at(2).toInt();
//        int height = message.arguments().at(3).toInt();
//        uint coordinateType = message.arguments().at(4).toUInt();
        qCDebug(lcAccessibilityAtspi) << "SetExtents is not implemented.";
        sendReply(connection, message, false);
    } else if (function == "SetPosition"_L1) {
//        int x = message.arguments().at(0).toInt();
//        int y = message.arguments().at(1).toInt();
//        uint coordinateType = message.arguments().at(2).toUInt();
        qCDebug(lcAccessibilityAtspi) << "SetPosition is not implemented.";
        sendReply(connection, message, false);
    } else if (function == "SetSize"_L1) {
//        int width = message.arguments().at(0).toInt();
//        int height = message.arguments().at(1).toInt();
        qCDebug(lcAccessibilityAtspi) << "SetSize is not implemented.";
        sendReply(connection, message, false);
    } else {
        qCWarning(lcAccessibilityAtspi) << "AtSpiAdaptor::componentInterface does not implement" << function << message.path();
        return false;
    }
    return true;
}

QRect AtSpiAdaptor::getExtents(QAccessibleInterface *interface, uint coordType)
{
    return translateFromScreenCoordinates(interface, interface->rect(), coordType);
}

// Action interface
bool AtSpiAdaptor::actionInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection)
{
    if (function == "GetNActions"_L1) {
        int count = QAccessibleBridgeUtils::effectiveActionNames(interface).size();
        sendReply(connection, message, QVariant::fromValue(QDBusVariant(QVariant::fromValue(count))));
    } else if (function == "DoAction"_L1) {
        int index = message.arguments().at(0).toInt();
        const QStringList actionNames = QAccessibleBridgeUtils::effectiveActionNames(interface);
        if (index < 0 || index >= actionNames.size())
            return false;
        const QString actionName = actionNames.at(index);
        bool success = QAccessibleBridgeUtils::performEffectiveAction(interface, actionName);
        sendReply(connection, message, success);
    } else if (function == "GetActions"_L1) {
        sendReply(connection, message, QVariant::fromValue(getActions(interface)));
    } else if (function == "GetName"_L1) {
        int index = message.arguments().at(0).toInt();
        const QStringList actionNames = QAccessibleBridgeUtils::effectiveActionNames(interface);
        if (index < 0 || index >= actionNames.size())
            return false;
        sendReply(connection, message, actionNames.at(index));
    } else if (function == "GetDescription"_L1) {
        int index = message.arguments().at(0).toInt();
        const QStringList actionNames = QAccessibleBridgeUtils::effectiveActionNames(interface);
        if (index < 0 || index >= actionNames.size())
            return false;
        QString description;
        if (QAccessibleActionInterface *actionIface = interface->actionInterface())
            description = actionIface->localizedActionDescription(actionNames.at(index));
        else
            description = qAccessibleLocalizedActionDescription(actionNames.at(index));
        sendReply(connection, message, description);
    } else if (function == "GetKeyBinding"_L1) {
        int index = message.arguments().at(0).toInt();
        const QStringList actionNames = QAccessibleBridgeUtils::effectiveActionNames(interface);
        if (index < 0 || index >= actionNames.size())
            return false;
        QStringList keyBindings;
        if (QAccessibleActionInterface *actionIface = interface->actionInterface())
            keyBindings = actionIface->keyBindingsForAction(actionNames.at(index));
        if (keyBindings.isEmpty()) {
            QString acc = interface->text(QAccessible::Accelerator);
            if (!acc.isEmpty())
                keyBindings.append(acc);
        }
        if (keyBindings.size() > 0)
            sendReply(connection, message, keyBindings.join(u';'));
        else
            sendReply(connection, message, QString());
    } else {
        qCWarning(lcAccessibilityAtspi) << "AtSpiAdaptor::actionInterface does not implement" << function << message.path();
        return false;
    }
    return true;
}

QSpiActionArray AtSpiAdaptor::getActions(QAccessibleInterface *interface) const
{
    QAccessibleActionInterface *actionInterface = interface->actionInterface();
    QSpiActionArray actions;
    const QStringList actionNames = QAccessibleBridgeUtils::effectiveActionNames(interface);
    actions.reserve(actionNames.size());
    for (const QString &actionName : actionNames) {
        QSpiAction action;

        action.name = actionName;
        if (actionInterface) {
            action.description = actionInterface->localizedActionDescription(actionName);
            const QStringList keyBindings = actionInterface->keyBindingsForAction(actionName);
            if (!keyBindings.isEmpty())
                action.keyBinding = keyBindings.front();
        } else {
            action.description = qAccessibleLocalizedActionDescription(actionName);
        }

        actions.append(std::move(action));
    }
    return actions;
}

// Text interface
bool AtSpiAdaptor::textInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection)
{
    if (!interface->textInterface())
        return false;

    // properties
    if (function == "GetCaretOffset"_L1) {
        sendReply(connection, message, QVariant::fromValue(QDBusVariant(QVariant::fromValue(interface->textInterface()->cursorPosition()))));
    } else if (function == "GetCharacterCount"_L1) {
        sendReply(connection, message, QVariant::fromValue(QDBusVariant(QVariant::fromValue(interface->textInterface()->characterCount()))));

    // functions
    } else if (function == "AddSelection"_L1) {
        int startOffset = message.arguments().at(0).toInt();
        int endOffset = message.arguments().at(1).toInt();
        int lastSelection = interface->textInterface()->selectionCount();
        interface->textInterface()->setSelection(lastSelection, startOffset, endOffset);
        sendReply(connection, message, (interface->textInterface()->selectionCount() > lastSelection));
    } else if (function == "GetAttributeRun"_L1) {
        int offset = message.arguments().at(0).toInt();
        bool includeDefaults = message.arguments().at(1).toBool();
        Q_UNUSED(includeDefaults);
        connection.send(message.createReply(getAttributes(interface, offset, includeDefaults)));
    } else if (function == "GetAttributeValue"_L1) {
        int offset = message.arguments().at(0).toInt();
        QString attributeName = message.arguments().at(1).toString();
        connection.send(message.createReply(QVariant(getAttributeValue(interface, offset, attributeName))));
    } else if (function == "GetAttributes"_L1) {
        int offset = message.arguments().at(0).toInt();
        connection.send(message.createReply(getAttributes(interface, offset, true)));
    } else if (function == "GetBoundedRanges"_L1) {
        int x = message.arguments().at(0).toInt();
        int y = message.arguments().at(1).toInt();
        int width = message.arguments().at(2).toInt();
        int height = message.arguments().at(3).toInt();
        uint coordType = message.arguments().at(4).toUInt();
        uint xClipType = message.arguments().at(5).toUInt();
        uint yClipType = message.arguments().at(6).toUInt();
        Q_UNUSED(x);
        Q_UNUSED(y);
        Q_UNUSED(width);
        Q_UNUSED(height);
        Q_UNUSED(coordType);
        Q_UNUSED(xClipType);
        Q_UNUSED(yClipType);
        qCDebug(lcAccessibilityAtspi) << "Not implemented: QSpiAdaptor::GetBoundedRanges";
        sendReply(connection, message, QVariant::fromValue(QSpiTextRangeList()));
    } else if (function == "GetCharacterAtOffset"_L1) {
        int offset = message.arguments().at(0).toInt();
        int start;
        int end;
        const QString charString = interface->textInterface()
                ->textAtOffset(offset, QAccessible::CharBoundary, &start, &end);
        int codePoint = 0;
        QStringIterator stringIt(charString);
        if (stringIt.hasNext())
            codePoint = static_cast<int>(stringIt.peekNext());
        sendReply(connection, message, codePoint);
    } else if (function == "GetCharacterExtents"_L1) {
        int offset = message.arguments().at(0).toInt();
        int coordType = message.arguments().at(1).toUInt();
        connection.send(message.createReply(getCharacterExtents(interface, offset, coordType)));
    } else if (function == "GetDefaultAttributeSet"_L1 || function == "GetDefaultAttributes"_L1) {
        // GetDefaultAttributes is deprecated in favour of GetDefaultAttributeSet.
        // Empty set seems reasonable. There is no default attribute set.
        sendReply(connection, message, QVariant::fromValue(QSpiAttributeSet()));
    } else if (function == "GetNSelections"_L1) {
        sendReply(connection, message, interface->textInterface()->selectionCount());
    } else if (function == "GetOffsetAtPoint"_L1) {
        qCDebug(lcAccessibilityAtspi) << message.signature();
        Q_ASSERT(!message.signature().isEmpty());
        QPoint point(message.arguments().at(0).toInt(), message.arguments().at(1).toInt());
        uint coordType = message.arguments().at(2).toUInt();
        if (!isValidCoordType(coordType))
            return false;
        QPoint screenPos = translateToScreenCoordinates(interface, point, coordType);
        int offset = interface->textInterface()->offsetAtPoint(screenPos);
        sendReply(connection, message, offset);
    } else if (function == "GetRangeExtents"_L1) {
        int startOffset = message.arguments().at(0).toInt();
        int endOffset = message.arguments().at(1).toInt();
        uint coordType = message.arguments().at(2).toUInt();
        connection.send(message.createReply(getRangeExtents(interface, startOffset, endOffset, coordType)));
    } else if (function == "GetSelection"_L1) {
        int selectionNum = message.arguments().at(0).toInt();
        int start, end;
        interface->textInterface()->selection(selectionNum, &start, &end);
        if (start < 0)
            start = end = interface->textInterface()->cursorPosition();
        QVariantList sel;
        sel << start << end;
        connection.send(message.createReply(sel));
    } else if (function == "GetStringAtOffset"_L1) {
        int offset = message.arguments().at(0).toInt();
        uint granularity = message.arguments().at(1).toUInt();
        if (!isValidAtspiTextGranularity(granularity))
            return false;
        int startOffset, endOffset;
        QString text = interface->textInterface()->textAtOffset(offset, qAccessibleBoundaryTypeFromAtspiTextGranularity(granularity), &startOffset, &endOffset);
        QVariantList ret;
        ret << text << startOffset << endOffset;
        connection.send(message.createReply(ret));
    } else if (function == "GetText"_L1) {
        int startOffset = message.arguments().at(0).toInt();
        int endOffset = message.arguments().at(1).toInt();
        if (endOffset == -1) // AT-SPI uses -1 to signal all characters
            endOffset = interface->textInterface()->characterCount();
        sendReply(connection, message, interface->textInterface()->text(startOffset, endOffset));
    } else if (function == "GetTextAfterOffset"_L1) {
        int offset = message.arguments().at(0).toInt();
        int type = message.arguments().at(1).toUInt();
        int startOffset, endOffset;
        QString text = interface->textInterface()->textAfterOffset(offset, qAccessibleBoundaryTypeFromAtspiBoundaryType(type), &startOffset, &endOffset);
        QVariantList ret;
        ret << text << startOffset << endOffset;
        connection.send(message.createReply(ret));
    } else if (function == "GetTextAtOffset"_L1) {
        int offset = message.arguments().at(0).toInt();
        int type = message.arguments().at(1).toUInt();
        int startOffset, endOffset;
        QString text = interface->textInterface()->textAtOffset(offset, qAccessibleBoundaryTypeFromAtspiBoundaryType(type), &startOffset, &endOffset);
        QVariantList ret;
        ret << text << startOffset << endOffset;
        connection.send(message.createReply(ret));
    } else if (function == "GetTextBeforeOffset"_L1) {
        int offset = message.arguments().at(0).toInt();
        int type = message.arguments().at(1).toUInt();
        int startOffset, endOffset;
        QString text = interface->textInterface()->textBeforeOffset(offset, qAccessibleBoundaryTypeFromAtspiBoundaryType(type), &startOffset, &endOffset);
        QVariantList ret;
        ret << text << startOffset << endOffset;
        connection.send(message.createReply(ret));
    } else if (function == "RemoveSelection"_L1) {
        int selectionNum = message.arguments().at(0).toInt();
        interface->textInterface()->removeSelection(selectionNum);
        sendReply(connection, message, true);
    } else if (function == "SetCaretOffset"_L1) {
        int offset = message.arguments().at(0).toInt();
        interface->textInterface()->setCursorPosition(offset);
        sendReply(connection, message, true);
    } else if (function == "ScrollSubstringTo"_L1) {
        int startOffset = message.arguments().at(0).toInt();
        int endOffset = message.arguments().at(1).toInt();
        // ignore third parameter (scroll type), since QAccessibleTextInterface::scrollToSubstring doesn't have that
        qCInfo(lcAccessibilityAtspi) << "AtSpiAdaptor::ScrollSubstringTo doesn'take take scroll type into account.";
        interface->textInterface()->scrollToSubstring(startOffset, endOffset);
        sendReply(connection, message, true);
    } else if (function == "SetSelection"_L1) {
        int selectionNum = message.arguments().at(0).toInt();
        int startOffset = message.arguments().at(1).toInt();
        int endOffset = message.arguments().at(2).toInt();
        interface->textInterface()->setSelection(selectionNum, startOffset, endOffset);
        sendReply(connection, message, true);
    } else {
        qCWarning(lcAccessibilityAtspi) << "AtSpiAdaptor::textInterface does not implement" << function << message.path();
        return false;
    }
    return true;
}

QAccessible::TextBoundaryType AtSpiAdaptor::qAccessibleBoundaryTypeFromAtspiBoundaryType(int atspiTextBoundaryType)
{
    switch (atspiTextBoundaryType) {
    case ATSPI_TEXT_BOUNDARY_CHAR:
        return QAccessible::CharBoundary;
    case ATSPI_TEXT_BOUNDARY_WORD_START:
    case ATSPI_TEXT_BOUNDARY_WORD_END:
        return QAccessible::WordBoundary;
    case ATSPI_TEXT_BOUNDARY_SENTENCE_START:
    case ATSPI_TEXT_BOUNDARY_SENTENCE_END:
        return QAccessible::SentenceBoundary;
    case ATSPI_TEXT_BOUNDARY_LINE_START:
    case ATSPI_TEXT_BOUNDARY_LINE_END:
        return QAccessible::LineBoundary;
    }
    Q_ASSERT_X(0, "", "Requested invalid boundary type.");
    return QAccessible::CharBoundary;
}

bool AtSpiAdaptor::isValidAtspiTextGranularity(uint atspiTextGranularity)
{
    if (atspiTextGranularity == ATSPI_TEXT_GRANULARITY_CHAR
            || atspiTextGranularity == ATSPI_TEXT_GRANULARITY_WORD
            || atspiTextGranularity == ATSPI_TEXT_GRANULARITY_SENTENCE
            || atspiTextGranularity == ATSPI_TEXT_GRANULARITY_LINE
            || atspiTextGranularity == ATSPI_TEXT_GRANULARITY_PARAGRAPH)
        return true;

    qCWarning(lcAccessibilityAtspi) << "Unknown value" << atspiTextGranularity << "for AT-SPI text granularity type";
    return false;
}

QAccessible::TextBoundaryType AtSpiAdaptor::qAccessibleBoundaryTypeFromAtspiTextGranularity(uint atspiTextGranularity)
{
    Q_ASSERT(isValidAtspiTextGranularity(atspiTextGranularity));

    switch (atspiTextGranularity) {
    case ATSPI_TEXT_GRANULARITY_CHAR:
        return QAccessible::CharBoundary;
    case ATSPI_TEXT_GRANULARITY_WORD:
        return QAccessible::WordBoundary;
    case ATSPI_TEXT_GRANULARITY_SENTENCE:
        return QAccessible::SentenceBoundary;
    case ATSPI_TEXT_GRANULARITY_LINE:
        return QAccessible::LineBoundary;
    case ATSPI_TEXT_GRANULARITY_PARAGRAPH:
        return QAccessible::ParagraphBoundary;
    }
    return QAccessible::CharBoundary;
}

namespace
{
    struct AtSpiAttribute {
        QString name;
        QString value;
        AtSpiAttribute(const QString &aName, const QString &aValue) : name(aName), value(aValue) {}
        bool isNull() const { return name.isNull() || value.isNull(); }
    };

    QString atspiColor(const QString &ia2Color)
    {
        // "rgb(%u,%u,%u)" -> "%u,%u,%u"
        return ia2Color.mid(4, ia2Color.size() - (4+1)).replace(u"\\,"_s, u","_s);
    }

    QString atspiSize(const QString &ia2Size)
    {
        // "%fpt" -> "%f"
        return ia2Size.left(ia2Size.size() - 2);
    }

    AtSpiAttribute atspiTextAttribute(const QString &ia2Name, const QString &ia2Value)
    {
        QString name = ia2Name;
        QString value = ia2Value;

        // IAccessible2: https://github.com/LinuxA11y/IAccessible2/blob/master/spec/textattributes.md
        // ATK attribute names: https://gitlab.gnome.org/GNOME/orca/-/blob/master/src/orca/text_attribute_names.py
        // ATK attribute values: https://gnome.pages.gitlab.gnome.org/atk/AtkText.html#AtkTextAttribute

        // https://bugzilla.gnome.org/show_bug.cgi?id=744553 "ATK docs provide no guidance for allowed values of some text attributes"
        // specifically for "weight", "invalid", "language" and value range for colors

        if (ia2Name == "background-color"_L1) {
            name = QStringLiteral("bg-color");
            value = atspiColor(value);
        } else if (ia2Name == "font-family"_L1) {
            name = QStringLiteral("family-name");
        } else if (ia2Name == "color"_L1) {
            name = QStringLiteral("fg-color");
            value = atspiColor(value);
        } else if (ia2Name == "text-align"_L1) {
            name = QStringLiteral("justification");
            if (value == "justify"_L1) {
                value = QStringLiteral("fill");
            } else if (value != "left"_L1 && value != "right"_L1 && value != "center"_L1) {
                qCDebug(lcAccessibilityAtspi) << "Unknown text-align attribute value \""
                                              << value << "\" cannot be translated to AT-SPI.";
                value = QString();
            }
        } else if (ia2Name == "font-size"_L1) {
            name = QStringLiteral("size");
            value = atspiSize(value);
        } else if (ia2Name == "font-style"_L1) {
            name = QStringLiteral("style");
            if (value != "normal"_L1 && value != "italic"_L1 &&  value != "oblique"_L1) {
                qCDebug(lcAccessibilityAtspi) << "Unknown font-style attribute value \"" << value
                                              << "\" cannot be translated to AT-SPI.";
                value = QString();
            }
        } else if (ia2Name == "text-underline-type"_L1) {
            name = QStringLiteral("underline");
            if (value != "none"_L1 && value != "single"_L1 && value != "double"_L1) {
                qCDebug(lcAccessibilityAtspi) << "Unknown text-underline-type attribute value \""
                                              << value << "\" cannot be translated to AT-SPI.";
                value = QString();
            }
        } else if (ia2Name == "font-weight"_L1) {
            name = QStringLiteral("weight");
            if (value == "normal"_L1)
                // Orca seems to accept all IAccessible2 values except for "normal"
                // (on which it produces traceback and fails to read any following text attributes),
                // but that is the default value, so omit it anyway
                value = QString();
        } else if (((ia2Name == "text-line-through-style"_L1 || ia2Name == "text-line-through-type"_L1) && (ia2Value != "none"_L1))
                   || (ia2Name == "text-line-through-text"_L1 && !ia2Value.isEmpty())) {
            // if any of the above is set, set "strikethrough" to true, but don't explicitly set
            // to false otherwise, since any of the others might still be set to indicate strikethrough
            // and no strikethrough is assumed anyway when nothing is explicitly set
            name = QStringLiteral("strikethrough");
            value = QStringLiteral("true");
        } else if (ia2Name == "text-position"_L1) {
            name = QStringLiteral("vertical-align");
            if (value != "baseline"_L1 && value != "super"_L1 && value != "sub"_L1) {
                qCDebug(lcAccessibilityAtspi) << "Unknown text-position attribute value \"" << value
                                              << "\" cannot be translated to AT-SPI.";
                value = QString();
            }
        } else if (ia2Name == "writing-mode"_L1) {
            name = QStringLiteral("direction");
            if (value == "lr"_L1)
                value = QStringLiteral("ltr");
            else if (value == "rl"_L1)
                value = QStringLiteral("rtl");
            else if (value == "tb"_L1) {
                // IAccessible2 docs refer to XSL, which specifies "tb" is shorthand for "tb-rl"; so at least give a hint about the horizontal direction (ATK does not support vertical direction in this attribute (yet))
                value = QStringLiteral("rtl");
                qCDebug(lcAccessibilityAtspi) << "writing-mode attribute value \"tb\" translated only w.r.t. horizontal direction; vertical direction ignored";
            } else {
                qCDebug(lcAccessibilityAtspi) << "Unknown writing-mode attribute value \"" << value
                                              << "\" cannot be translated to AT-SPI.";
                value = QString();
            }
        } else if (ia2Name == "language"_L1) {
            // OK - ATK has no docs on the format of the value, IAccessible2 has reasonable format - leave it at that now
        } else if (ia2Name == "invalid"_L1) {
            // OK - ATK docs are vague but suggest they support the same range of values as IAccessible2
        } else {
            // attribute we know nothing about
            name = QString();
            value = QString();
        }
        return AtSpiAttribute(name, value);
    }
}

QSpiAttributeSet AtSpiAdaptor::getAttributes(QAccessibleInterface *interface) const
{
    QSpiAttributeSet set;
    QAccessibleAttributesInterface *attributesIface = interface->attributesInterface();
    if (!attributesIface)
        return set;

    const QList<QAccessible::Attribute> attrKeys = attributesIface->attributeKeys();
    for (QAccessible::Attribute key : attrKeys) {
        const QVariant value = attributesIface->attributeValue(key);
        // see "Core Accessibility API Mappings" spec: https://www.w3.org/TR/core-aam-1.2/
        switch (key) {
        case QAccessible::Attribute::Custom:
        {
            // forward custom attributes to AT-SPI as-is
            Q_ASSERT((value.canConvert<QHash<QString, QString>>()));
            const QHash<QString, QString> attrMap = value.value<QHash<QString, QString>>();
            for (auto [name, val] : attrMap.asKeyValueRange())
                set.insert(name, val);
            break;
        }
        case QAccessible::Attribute::Level:
            Q_ASSERT(value.canConvert<int>());
            set.insert(QStringLiteral("level"), QString::number(value.toInt()));
            break;
        default:
            break;
        }
    }
    return set;
}

// FIXME all attribute methods below should share code
QVariantList AtSpiAdaptor::getAttributes(QAccessibleInterface *interface, int offset, bool includeDefaults) const
{
    Q_UNUSED(includeDefaults);

    QSpiAttributeSet set;
    int startOffset;
    int endOffset;

    QString joined = interface->textInterface()->attributes(offset, &startOffset, &endOffset);
    const QStringList attributes = joined.split(u';', Qt::SkipEmptyParts, Qt::CaseSensitive);
    for (const QString &attr : attributes) {
        QStringList items = attr.split(u':', Qt::SkipEmptyParts, Qt::CaseSensitive);
        if (items.count() == 2)
        {
            AtSpiAttribute attribute = atspiTextAttribute(items[0], items[1]);
            if (!attribute.isNull())
                set[attribute.name] = attribute.value;
        }
    }

    QVariantList list;
    list << QVariant::fromValue(set) << startOffset << endOffset;

    return list;
}

QString AtSpiAdaptor::getAttributeValue(QAccessibleInterface *interface, int offset, const QString &attributeName) const
{
    QString joined;
    QSpiAttributeSet map;
    int startOffset;
    int endOffset;

    joined = interface->textInterface()->attributes(offset, &startOffset, &endOffset);
    const QStringList attributes = joined.split (u';', Qt::SkipEmptyParts, Qt::CaseSensitive);
    for (const QString& attr : attributes) {
        QStringList items;
        items = attr.split(u':', Qt::SkipEmptyParts, Qt::CaseSensitive);
        AtSpiAttribute attribute = atspiTextAttribute(items[0], items[1]);
        if (!attribute.isNull())
            map[attribute.name] = attribute.value;
    }
    return map[attributeName];
}

QList<QVariant> AtSpiAdaptor::getCharacterExtents(QAccessibleInterface *interface, int offset, uint coordType) const
{
    QRect rect = interface->textInterface()->characterRect(offset);
    rect = translateFromScreenCoordinates(interface, rect, coordType);
    return QList<QVariant>() << rect.x() << rect.y() << rect.width() << rect.height();
}

QList<QVariant> AtSpiAdaptor::getRangeExtents(QAccessibleInterface *interface,
                                            int startOffset, int endOffset, uint coordType) const
{
    if (endOffset == -1)
        endOffset = interface->textInterface()->characterCount();

    QAccessibleTextInterface *textInterface = interface->textInterface();
    if (endOffset <= startOffset || !textInterface)
        return QList<QVariant>() << -1 << -1 << 0 << 0;

    QRect rect = textInterface->characterRect(startOffset);
    for (int i=startOffset + 1; i <= endOffset; i++)
        rect = rect | textInterface->characterRect(i);

    rect = translateFromScreenCoordinates(interface, rect, coordType);
    return QList<QVariant>() << rect.x() << rect.y() << rect.width() << rect.height();
}

bool AtSpiAdaptor::isValidCoordType(uint coordType)
{
    if (coordType == ATSPI_COORD_TYPE_SCREEN || coordType == ATSPI_COORD_TYPE_WINDOW || coordType == ATSPI_COORD_TYPE_PARENT)
        return true;

    qCWarning(lcAccessibilityAtspi) << "Unknown value" << coordType << "for AT-SPI coord type";
    return false;
}

QRect AtSpiAdaptor::translateFromScreenCoordinates(QAccessibleInterface *interface, const QRect &screenRect, uint targetCoordType)
{
    Q_ASSERT(isValidCoordType(targetCoordType));

    QAccessibleInterface *upper = nullptr;
    if (targetCoordType == ATSPI_COORD_TYPE_WINDOW)
        upper = getWindow(interface);
    else if (targetCoordType == ATSPI_COORD_TYPE_PARENT)
        upper = interface->parent();

    QRect rect = screenRect;
    if (upper)
        rect.translate(-upper->rect().x(), -upper->rect().y());

    return rect;
}

QPoint AtSpiAdaptor::translateToScreenCoordinates(QAccessibleInterface *interface, const QPoint &pos, uint fromCoordType)
{
    Q_ASSERT(isValidCoordType(fromCoordType));

    QAccessibleInterface *upper = nullptr;
    if (fromCoordType == ATSPI_COORD_TYPE_WINDOW)
        upper = getWindow(interface);
    else if (fromCoordType == ATSPI_COORD_TYPE_PARENT)
        upper = interface->parent();

    QPoint screenPos = pos;
    if (upper)
        screenPos += upper->rect().topLeft();

    return screenPos;
}

// Editable Text interface
static QString textForRange(QAccessibleInterface *accessible, int startOffset, int endOffset)
{
    if (QAccessibleTextInterface *textIface = accessible->textInterface()) {
        if (endOffset == -1)
            endOffset = textIface->characterCount();
        return textIface->text(startOffset, endOffset);
    }
    QString txt = accessible->text(QAccessible::Value);
    if (endOffset == -1)
        endOffset = txt.size();
    return txt.mid(startOffset, endOffset - startOffset);
}

static void replaceTextFallback(QAccessibleInterface *accessible, long startOffset, long endOffset, const QString &txt)
{
    QString t = textForRange(accessible, 0, -1);
    if (endOffset == -1)
        endOffset = t.size();
    if (endOffset - startOffset == 0)
        t.insert(startOffset, txt);
    else
        t.replace(startOffset, endOffset - startOffset, txt);
    accessible->setText(QAccessible::Value, t);
}

bool AtSpiAdaptor::editableTextInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection)
{
    if (function == "CopyText"_L1) {
#ifndef QT_NO_CLIPBOARD
        int startOffset = message.arguments().at(0).toInt();
        int endOffset = message.arguments().at(1).toInt();
        const QString t = textForRange(interface, startOffset, endOffset);
        QGuiApplication::clipboard()->setText(t);
#endif
        connection.send(message.createReply(true));
    } else if (function == "CutText"_L1) {
#ifndef QT_NO_CLIPBOARD
        int startOffset = message.arguments().at(0).toInt();
        int endOffset = message.arguments().at(1).toInt();
        const QString t = textForRange(interface, startOffset, endOffset);
        if (QAccessibleEditableTextInterface *editableTextIface = interface->editableTextInterface())
            editableTextIface->deleteText(startOffset, endOffset);
        else
            replaceTextFallback(interface, startOffset, endOffset, QString());
        QGuiApplication::clipboard()->setText(t);
#endif
        connection.send(message.createReply(true));
    } else if (function == "DeleteText"_L1) {
        int startOffset = message.arguments().at(0).toInt();
        int endOffset = message.arguments().at(1).toInt();
        if (QAccessibleEditableTextInterface *editableTextIface = interface->editableTextInterface())
            editableTextIface->deleteText(startOffset, endOffset);
        else
            replaceTextFallback(interface, startOffset, endOffset, QString());
        connection.send(message.createReply(true));
    } else if (function == "InsertText"_L1) {
        int position = message.arguments().at(0).toInt();
        QString text = message.arguments().at(1).toString();
        int length = message.arguments().at(2).toInt();
        text.resize(length);
        if (QAccessibleEditableTextInterface *editableTextIface = interface->editableTextInterface())
            editableTextIface->insertText(position, text);
        else
            replaceTextFallback(interface, position, position, text);
        connection.send(message.createReply(true));
    } else if (function == "PasteText"_L1) {
#ifndef QT_NO_CLIPBOARD
        int position = message.arguments().at(0).toInt();
        const QString txt = QGuiApplication::clipboard()->text();
        if (QAccessibleEditableTextInterface *editableTextIface = interface->editableTextInterface())
            editableTextIface->insertText(position, txt);
        else
            replaceTextFallback(interface, position, position, txt);
#endif
        connection.send(message.createReply(true));
    } else if (function == "SetTextContents"_L1) {
        QString newContents = message.arguments().at(0).toString();
        if (QAccessibleEditableTextInterface *editableTextIface = interface->editableTextInterface())
            editableTextIface->replaceText(0, interface->textInterface()->characterCount(), newContents);
        else
            replaceTextFallback(interface, 0, -1, newContents);
        connection.send(message.createReply(true));
    } else if (function.isEmpty()) {
        connection.send(message.createReply());
    } else {
        qCWarning(lcAccessibilityAtspi) << "AtSpiAdaptor::editableTextInterface does not implement" << function << message.path();
        return false;
    }
    return true;
}

// Value interface
bool AtSpiAdaptor::valueInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection)
{
    QAccessibleValueInterface *valueIface = interface->valueInterface();
    if (!valueIface)
        return false;

    if (function == "SetCurrentValue"_L1) {
        QDBusVariant v = qvariant_cast<QDBusVariant>(message.arguments().at(2));
        double value = v.variant().toDouble();
        //Temporary fix
        //See https://bugzilla.gnome.org/show_bug.cgi?id=652596
        valueIface->setCurrentValue(value);
        connection.send(message.createReply());
    } else {
        QVariant value;
        if (function == "GetCurrentValue"_L1)
            value = valueIface->currentValue();
        else if (function == "GetMaximumValue"_L1)
            value = valueIface->maximumValue();
        else if (function == "GetMinimumIncrement"_L1)
            value = valueIface->minimumStepSize();
        else if (function == "GetMinimumValue"_L1)
            value = valueIface->minimumValue();
        else {
            qCWarning(lcAccessibilityAtspi) << "AtSpiAdaptor::valueInterface does not implement" << function << message.path();
            return false;
        }
        if (!value.canConvert<double>()) {
            qCWarning(lcAccessibilityAtspi) << "AtSpiAdaptor::valueInterface: Could not convert to double:" << function;
        }

        // explicitly convert to dbus-variant containing one double since atspi expects that
        // everything else might fail to convert back on the other end
        connection.send(message.createReply(
                            QVariant::fromValue(QDBusVariant(QVariant::fromValue(value.toDouble())))));
    }
    return true;
}

// Selection interface
bool AtSpiAdaptor::selectionInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection)
{
    QAccessibleSelectionInterface* selectionInterface = interface->selectionInterface();
    if (!selectionInterface) {
        qCWarning(lcAccessibilityAtspi) << "Could not find selection interface for: " << message.path() << interface;
        return false;
    }

    if (function == "ClearSelection"_L1 ) {
        connection.send(message.createReply(QVariant::fromValue((selectionInterface->clear()))));
    } else if (function == "DeselectChild"_L1 ) {
        int childIndex = message.arguments().at(0).toInt();
        bool ret = false;
        QAccessibleInterface *child = interface->child(childIndex);
        if (child)
            ret = selectionInterface->unselect(child);
        connection.send(message.createReply(QVariant::fromValue(ret)));
    } else if (function == "DeselectSelectedChild"_L1 ) {
        int selectionIndex = message.arguments().at(0).toInt();
        bool ret = false;
        QAccessibleInterface *selectedChild = selectionInterface->selectedItem(selectionIndex);
        if (selectedChild)
            ret = selectionInterface->unselect(selectedChild);
        connection.send(message.createReply(QVariant::fromValue(ret)));
    } else if (function == "GetNSelectedChildren"_L1) {
        connection.send(message.createReply(QVariant::fromValue(QDBusVariant(
            QVariant::fromValue(selectionInterface->selectedItemCount())))));
    } else if (function == "GetSelectedChild"_L1) {
        int selectionIndex = message.arguments().at(0).toInt();
        QSpiObjectReference ref(connection, QDBusObjectPath(pathForInterface(selectionInterface->selectedItem(selectionIndex))));
        connection.send(message.createReply(QVariant::fromValue(ref)));
    } else if (function == "IsChildSelected"_L1 ) {
        int childIndex = message.arguments().at(0).toInt();
        bool ret = false;
        QAccessibleInterface *child = interface->child(childIndex);
        if (child)
            ret = selectionInterface->isSelected(child);
        connection.send(message.createReply(QVariant::fromValue(ret)));
    } else if (function == "SelectAll"_L1 ) {
        connection.send(message.createReply(QVariant::fromValue(selectionInterface->selectAll())));
    } else if (function == "SelectChild"_L1 ) {
        int childIndex = message.arguments().at(0).toInt();
        bool ret = false;
        QAccessibleInterface *child = interface->child(childIndex);
        if (child)
            ret = selectionInterface->select(child);
        connection.send(message.createReply(QVariant::fromValue(ret)));
    } else {
        qCWarning(lcAccessibilityAtspi) << "AtSpiAdaptor::selectionInterface does not implement " << function << message.path();
        return false;
    }

    return true;
}


// Table interface
bool AtSpiAdaptor::tableInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection)
{
    if (!(interface->tableInterface() || interface->tableCellInterface())) {
        qCWarning(lcAccessibilityAtspi) << "Qt AtSpiAdaptor: Could not find table interface for:" << message.path() << interface;
        return false;
    }

    if (function == "GetCaption"_L1) {
        QAccessibleInterface * captionInterface= interface->tableInterface()->caption();
        if (captionInterface) {
            QSpiObjectReference ref = QSpiObjectReference(connection, QDBusObjectPath(pathForInterface(captionInterface)));
            sendReply(connection, message, QVariant::fromValue(QDBusVariant(QVariant::fromValue(ref))));
        } else {
            sendReply(connection, message, QVariant::fromValue(QDBusVariant(QVariant::fromValue(
                          QSpiObjectReference(connection, QDBusObjectPath(ATSPI_DBUS_PATH_NULL))))));
        }
    } else if (function == "GetNColumns"_L1) {
        connection.send(message.createReply(QVariant::fromValue(QDBusVariant(
            QVariant::fromValue(interface->tableInterface()->columnCount())))));
    } else if (function == "GetNRows"_L1) {
        connection.send(message.createReply(QVariant::fromValue(QDBusVariant(
            QVariant::fromValue(interface->tableInterface()->rowCount())))));
    } else if (function == "GetNSelectedColumns"_L1) {
        connection.send(message.createReply(QVariant::fromValue(QDBusVariant(
            QVariant::fromValue(interface->tableInterface()->selectedColumnCount())))));
    } else if (function == "GetNSelectedRows"_L1) {
        connection.send(message.createReply(QVariant::fromValue(QDBusVariant(
            QVariant::fromValue(interface->tableInterface()->selectedRowCount())))));
    } else if (function == "GetSummary"_L1) {
        QAccessibleInterface *summary = interface->tableInterface() ? interface->tableInterface()->summary() : nullptr;
        QSpiObjectReference ref(connection, QDBusObjectPath(pathForInterface(summary)));
        connection.send(message.createReply(QVariant::fromValue(QDBusVariant(QVariant::fromValue(ref)))));
    } else if (function == "GetAccessibleAt"_L1) {
        int row = message.arguments().at(0).toInt();
        int column = message.arguments().at(1).toInt();
        if ((row < 0) ||
            (column < 0) ||
            (row >= interface->tableInterface()->rowCount()) ||
            (column >= interface->tableInterface()->columnCount())) {
            qCWarning(lcAccessibilityAtspi) << "Invalid index for tableInterface GetAccessibleAt (" << row << "," << column << ')';
            return false;
        }

        QSpiObjectReference ref;
        QAccessibleInterface * cell(interface->tableInterface()->cellAt(row, column));
        if (cell) {
            ref = QSpiObjectReference(connection, QDBusObjectPath(pathForInterface(cell)));
        } else {
            qCWarning(lcAccessibilityAtspi) << "No cell interface returned for" << interface->object() << row << column;
            ref = QSpiObjectReference();
        }
        connection.send(message.createReply(QVariant::fromValue(ref)));

    } else if (function == "GetIndexAt"_L1) {
        int row = message.arguments().at(0).toInt();
        int column = message.arguments().at(1).toInt();
        QAccessibleInterface *cell = interface->tableInterface()->cellAt(row, column);
        if (!cell) {
            qCWarning(lcAccessibilityAtspi) << "AtSpiAdaptor::GetIndexAt(" << row << ',' << column << ") did not find a cell." << interface;
            return false;
        }
        int index = interface->indexOfChild(cell);
        qCDebug(lcAccessibilityAtspi) << "QSpiAdaptor::GetIndexAt row:" << row << " col:" << column << " logical index:" << index;
        Q_ASSERT(index > 0);
        connection.send(message.createReply(index));
    } else if ((function == "GetColumnAtIndex"_L1) || (function == "GetRowAtIndex"_L1)) {
        int index = message.arguments().at(0).toInt();
        int ret = -1;
        if (index >= 0) {
            QAccessibleInterface * cell = interface->child(index);
            if (cell) {
                if (function == "GetColumnAtIndex"_L1) {
                    if (cell->role() == QAccessible::ColumnHeader) {
                        ret = index;
                    } else if (cell->role() == QAccessible::RowHeader) {
                        ret = -1;
                    } else {
                        if (!cell->tableCellInterface()) {
                            qCWarning(lcAccessibilityAtspi).nospace() << "AtSpiAdaptor::" << function << " No table cell interface: " << cell;
                            return false;
                        }
                        ret = cell->tableCellInterface()->columnIndex();
                    }
                } else {
                    if (cell->role() == QAccessible::ColumnHeader) {
                        ret = -1;
                    } else if (cell->role() == QAccessible::RowHeader) {
                        ret = index % interface->tableInterface()->columnCount();
                    } else {
                        if (!cell->tableCellInterface()) {
                            qCWarning(lcAccessibilityAtspi).nospace() << "AtSpiAdaptor::" << function << " No table cell interface: " << cell;
                            return false;
                        }
                        ret = cell->tableCellInterface()->rowIndex();
                    }
                }
            } else {
                qCWarning(lcAccessibilityAtspi).nospace() << "AtSpiAdaptor::" << function << " No cell at index: " << index << " " << interface;
                return false;
            }
        }
        connection.send(message.createReply(ret));

    } else if (function == "GetColumnDescription"_L1) {
        int column = message.arguments().at(0).toInt();
        connection.send(message.createReply(interface->tableInterface()->columnDescription(column)));
    } else if (function == "GetRowDescription"_L1) {
        int row = message.arguments().at(0).toInt();
        connection.send(message.createReply(interface->tableInterface()->rowDescription(row)));



    } else if (function == "GetRowColumnExtentsAtIndex"_L1) {
        int index = message.arguments().at(0).toInt();
        bool success = false;

        int row = -1;
        int col = -1;
        int rowExtents = -1;
        int colExtents = -1;
        bool isSelected = false;

        int cols = interface->tableInterface()->columnCount();
        if (cols > 0) {
            row = index / cols;
            col = index % cols;
            if (QAccessibleInterface *cell = interface->tableInterface()->cellAt(row, col)) {
                if (QAccessibleTableCellInterface *cellIface = cell->tableCellInterface()) {
                    row = cellIface->rowIndex();
                    col = cellIface->columnIndex();
                    rowExtents = cellIface->rowExtent();
                    colExtents = cellIface->columnExtent();
                    isSelected = cellIface->isSelected();
                    success = true;
                }
            }
        }
        QVariantList list;
        list << success << row << col << rowExtents << colExtents << isSelected;
        connection.send(message.createReply(list));

    } else if (function == "GetColumnExtentAt"_L1) {
        int row = message.arguments().at(0).toInt();
        int column = message.arguments().at(1).toInt();
        int columnExtent = 0;
        if (QAccessibleInterface *cell = interface->tableInterface()->cellAt(row, column)) {
            if (QAccessibleTableCellInterface *cellIface = cell->tableCellInterface())
                columnExtent = cellIface->columnExtent();
        }
        connection.send(message.createReply(columnExtent));

    } else if (function == "GetRowExtentAt"_L1) {
        int row = message.arguments().at(0).toInt();
        int column = message.arguments().at(1).toInt();
        int rowExtent = 0;
        if (QAccessibleInterface *cell = interface->tableInterface()->cellAt(row, column)) {
            if (QAccessibleTableCellInterface *cellIface = cell->tableCellInterface())
                rowExtent = cellIface->rowExtent();
        }
        connection.send(message.createReply(rowExtent));

    } else if (function == "GetColumnHeader"_L1) {
        int column = message.arguments().at(0).toInt();
        QSpiObjectReference ref;

        QAccessibleInterface * cell(interface->tableInterface()->cellAt(0, column));
        if (cell && cell->tableCellInterface()) {
            QList<QAccessibleInterface*> header = cell->tableCellInterface()->columnHeaderCells();
            if (header.size() > 0) {
                ref = QSpiObjectReference(connection, QDBusObjectPath(pathForInterface(header.takeAt(0))));
            }
        }
        connection.send(message.createReply(QVariant::fromValue(ref)));

    } else if (function == "GetRowHeader"_L1) {
        int row = message.arguments().at(0).toInt();
        QSpiObjectReference ref;
        QAccessibleInterface *cell = interface->tableInterface()->cellAt(row, 0);
        if (cell && cell->tableCellInterface()) {
            QList<QAccessibleInterface*> header = cell->tableCellInterface()->rowHeaderCells();
            if (header.size() > 0) {
                ref = QSpiObjectReference(connection, QDBusObjectPath(pathForInterface(header.takeAt(0))));
            }
        }
        connection.send(message.createReply(QVariant::fromValue(ref)));

    } else if (function == "GetSelectedColumns"_L1) {
        connection.send(message.createReply(QVariant::fromValue(interface->tableInterface()->selectedColumns())));
    } else if (function == "GetSelectedRows"_L1) {
        connection.send(message.createReply(QVariant::fromValue(interface->tableInterface()->selectedRows())));
    } else if (function == "IsColumnSelected"_L1) {
        int column = message.arguments().at(0).toInt();
        connection.send(message.createReply(interface->tableInterface()->isColumnSelected(column)));
    } else if (function == "IsRowSelected"_L1) {
        int row = message.arguments().at(0).toInt();
        connection.send(message.createReply(interface->tableInterface()->isRowSelected(row)));
    } else if (function == "IsSelected"_L1) {
        int row = message.arguments().at(0).toInt();
        int column = message.arguments().at(1).toInt();
        bool selected = false;
        if (QAccessibleInterface* cell = interface->tableInterface()->cellAt(row, column)) {
            if (QAccessibleTableCellInterface *cellIface = cell->tableCellInterface())
                selected = cellIface->isSelected();
        }
        connection.send(message.createReply(selected));
    } else if (function == "AddColumnSelection"_L1) {
        int column = message.arguments().at(0).toInt();
        connection.send(message.createReply(interface->tableInterface()->selectColumn(column)));
    } else if (function == "AddRowSelection"_L1) {
        int row = message.arguments().at(0).toInt();
        connection.send(message.createReply(interface->tableInterface()->selectRow(row)));
    } else if (function == "RemoveColumnSelection"_L1) {
        int column = message.arguments().at(0).toInt();
        connection.send(message.createReply(interface->tableInterface()->unselectColumn(column)));
    } else if (function ==  "RemoveRowSelection"_L1) {
        int row = message.arguments().at(0).toInt();
        connection.send(message.createReply(interface->tableInterface()->unselectRow(row)));
    } else {
        qCWarning(lcAccessibilityAtspi) << "AtSpiAdaptor::tableInterface does not implement" << function << message.path();
        return false;
    }
    return true;
}

// Table cell interface
bool AtSpiAdaptor::tableCellInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection)
{
    QAccessibleTableCellInterface* cellInterface = interface->tableCellInterface();
    if (!cellInterface) {
        qCWarning(lcAccessibilityAtspi) << "Could not find table cell interface for: " << message.path() << interface;
        return false;
    }

    if (function == "GetColumnHeaderCells"_L1) {
        QSpiObjectReferenceArray headerCells;
        const auto headerCellInterfaces = cellInterface->columnHeaderCells();
        headerCells.reserve(headerCellInterfaces.size());
        for (QAccessibleInterface *cell : headerCellInterfaces) {
            const QString childPath = pathForInterface(cell);
            const QSpiObjectReference ref(connection, QDBusObjectPath(childPath));
            headerCells << ref;
        }
        connection.send(message.createReply(QVariant::fromValue(headerCells)));
    } else if (function == "GetColumnSpan"_L1) {
        connection.send(message.createReply(QVariant::fromValue(QDBusVariant(
            QVariant::fromValue(cellInterface->columnExtent())))));
    } else if (function == "GetPosition"_L1) {
        const int row = cellInterface->rowIndex();
        const int column = cellInterface->columnIndex();
        connection.send(message.createReply(QVariant::fromValue(QDBusVariant(
            QVariant::fromValue(QPoint(row, column))))));
    } else if (function == "GetRowHeaderCells"_L1) {
        QSpiObjectReferenceArray headerCells;
        const auto headerCellInterfaces = cellInterface->rowHeaderCells();
        headerCells.reserve(headerCellInterfaces.size());
        for (QAccessibleInterface *cell : headerCellInterfaces) {
            const QString childPath = pathForInterface(cell);
            const QSpiObjectReference ref(connection, QDBusObjectPath(childPath));
            headerCells << ref;
        }
        connection.send(message.createReply(QVariant::fromValue(headerCells)));
    } else if (function == "GetRowSpan"_L1) {
        connection.send(message.createReply(QVariant::fromValue(QDBusVariant(
            QVariant::fromValue(cellInterface->rowExtent())))));
    } else if (function == "GetRowColumnSpan"_L1) {
        QVariantList list;
        list << cellInterface->rowIndex() << cellInterface->columnIndex() << cellInterface->rowExtent() << cellInterface->columnExtent();
        connection.send(message.createReply(list));
    } else if (function == "GetTable"_L1) {
        QSpiObjectReference ref;
        QAccessibleInterface* table = cellInterface->table();
        if (table && table->tableInterface())
            ref = QSpiObjectReference(connection, QDBusObjectPath(pathForInterface(table)));
        connection.send(message.createReply(QVariant::fromValue(QDBusVariant(QVariant::fromValue(ref)))));
    } else {
        qCWarning(lcAccessibilityAtspi) << "AtSpiAdaptor::tableCellInterface does not implement" << function << message.path();
        return false;
    }

    return true;
}

QT_END_NAMESPACE

#include "moc_atspiadaptor_p.cpp"
#endif // QT_CONFIG(accessibility)
