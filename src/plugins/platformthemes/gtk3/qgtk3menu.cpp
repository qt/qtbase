/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "qgtk3menu.h"

#include <QtGui/qwindow.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/qpa/qplatformwindow.h>

#undef signals
#include <gtk/gtk.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(shortcut)
static guint qt_gdkKey(const QKeySequence &shortcut)
{
    if (shortcut.isEmpty())
        return 0;

    // TODO: proper mapping
    Qt::KeyboardModifiers mods = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;
    return (shortcut[0] ^ mods) & shortcut[0];
}

static GdkModifierType qt_gdkModifiers(const QKeySequence &shortcut)
{
    if (shortcut.isEmpty())
        return GdkModifierType(0);

    guint mods = 0;
    int m = shortcut[0];
    if (m & Qt::ShiftModifier)
        mods |= GDK_SHIFT_MASK;
    if (m & Qt::ControlModifier)
        mods |= GDK_CONTROL_MASK;
    if (m & Qt::AltModifier)
        mods |= GDK_MOD1_MASK;
    if (m & Qt::MetaModifier)
        mods |= GDK_META_MASK;

    return static_cast<GdkModifierType>(mods);
}
#endif

QGtk3MenuItem::QGtk3MenuItem()
    : m_visible(true),
      m_separator(false),
      m_checkable(false),
      m_checked(false),
      m_enabled(true),
      m_underline(false),
      m_invalid(true),
      m_menu(nullptr),
      m_item(nullptr)
{
}

QGtk3MenuItem::~QGtk3MenuItem()
{
}

bool QGtk3MenuItem::isInvalid() const
{
    return m_invalid;
}

GtkWidget *QGtk3MenuItem::create()
{
    if (m_invalid) {
        if (m_item) {
            gtk_widget_destroy(m_item);
            m_item = nullptr;
        }
        m_invalid = false;
    }

    if (!m_item) {
        if (m_separator) {
            m_item = gtk_separator_menu_item_new();
        } else {
            if (m_checkable) {
                m_item = gtk_check_menu_item_new();
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m_item), m_checked);
                g_signal_connect(m_item, "toggled", G_CALLBACK(onToggle), this);
            } else {
                m_item = gtk_menu_item_new();
                g_signal_connect(m_item, "activate", G_CALLBACK(onActivate), this);
            }
            gtk_menu_item_set_label(GTK_MENU_ITEM(m_item), m_text.toUtf8());
            gtk_menu_item_set_use_underline(GTK_MENU_ITEM(m_item), m_underline);
            if (m_menu)
                gtk_menu_item_set_submenu(GTK_MENU_ITEM(m_item), m_menu->handle());
            g_signal_connect(m_item, "select", G_CALLBACK(onSelect), this);
#if QT_CONFIG(shortcut)
            if (!m_shortcut.isEmpty()) {
                GtkWidget *label = gtk_bin_get_child(GTK_BIN(m_item));
                gtk_accel_label_set_accel(GTK_ACCEL_LABEL(label), qt_gdkKey(m_shortcut), qt_gdkModifiers(m_shortcut));
            }
#endif
        }
        gtk_widget_set_sensitive(m_item, m_enabled);
        gtk_widget_set_visible(m_item, m_visible);
        if (GTK_IS_CHECK_MENU_ITEM(m_item))
            g_object_set(m_item, "draw-as-radio", m_exclusive, NULL);
    }

    return m_item;
}

GtkWidget *QGtk3MenuItem::handle() const
{
    return m_item;
}

QString QGtk3MenuItem::text() const
{
    return m_text;
}

static QString convertMnemonics(QString text, bool *found)
{
    *found = false;

    int i = text.length() - 1;
    while (i >= 0) {
        const QChar c = text.at(i);
        if (c == QLatin1Char('&')) {
            if (i == 0 || text.at(i - 1) != QLatin1Char('&')) {
                // convert Qt to GTK mnemonic
                if (i < text.length() - 1 && !text.at(i + 1).isSpace()) {
                    text.replace(i, 1, QLatin1Char('_'));
                    *found = true;
                }
            } else if (text.at(i - 1) == QLatin1Char('&')) {
                // unescape ampersand
                text.replace(--i, 2, QLatin1Char('&'));
            }
        } else if (c == QLatin1Char('_')) {
            // escape GTK mnemonic
            text.insert(i, QLatin1Char('_'));
        }
        --i;
    }

    return text;
}

void QGtk3MenuItem::setText(const QString &text)
{
    m_text = convertMnemonics(text, &m_underline);
    if (GTK_IS_MENU_ITEM(m_item)) {
        gtk_menu_item_set_label(GTK_MENU_ITEM(m_item), m_text.toUtf8());
        gtk_menu_item_set_use_underline(GTK_MENU_ITEM(m_item), m_underline);
    }
}

QGtk3Menu *QGtk3MenuItem::menu() const
{
    return m_menu;
}

void QGtk3MenuItem::setMenu(QPlatformMenu *menu)
{
    m_menu = qobject_cast<QGtk3Menu *>(menu);
    if (GTK_IS_MENU_ITEM(m_item))
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(m_item), m_menu ? m_menu->handle() : NULL);
}

bool QGtk3MenuItem::isVisible() const
{
    return m_visible;
}

void QGtk3MenuItem::setVisible(bool visible)
{
    if (m_visible == visible)
        return;

    m_visible = visible;
    if (GTK_IS_MENU_ITEM(m_item))
        gtk_widget_set_visible(m_item, visible);
}

bool QGtk3MenuItem::isSeparator() const
{
    return m_separator;
}

void QGtk3MenuItem::setIsSeparator(bool separator)
{
    if (m_separator == separator)
        return;

    m_invalid = true;
    m_separator = separator;
}

bool QGtk3MenuItem::isCheckable() const
{
    return m_checkable;
}

void QGtk3MenuItem::setCheckable(bool checkable)
{
    if (m_checkable == checkable)
        return;

    m_invalid = true;
    m_checkable = checkable;
}

bool QGtk3MenuItem::isChecked() const
{
    return m_checked;
}

void QGtk3MenuItem::setChecked(bool checked)
{
    if (m_checked == checked)
        return;

    m_checked = checked;
    if (GTK_IS_CHECK_MENU_ITEM(m_item))
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m_item), checked);
}

#if QT_CONFIG(shortcut)
QKeySequence QGtk3MenuItem::shortcut() const
{
    return m_shortcut;
}

void QGtk3MenuItem::setShortcut(const QKeySequence& shortcut)
{
    if (m_shortcut == shortcut)
        return;

    m_shortcut = shortcut;
    if (GTK_IS_MENU_ITEM(m_item)) {
        GtkWidget *label = gtk_bin_get_child(GTK_BIN(m_item));
        gtk_accel_label_set_accel(GTK_ACCEL_LABEL(label), qt_gdkKey(m_shortcut), qt_gdkModifiers(m_shortcut));
    }
}
#endif

bool QGtk3MenuItem::isEnabled() const
{
    return m_enabled;
}

void QGtk3MenuItem::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
    if (m_item)
        gtk_widget_set_sensitive(m_item, enabled);
}

bool QGtk3MenuItem::hasExclusiveGroup() const
{
    return m_exclusive;
}

void QGtk3MenuItem::setHasExclusiveGroup(bool exclusive)
{
    if (m_exclusive == exclusive)
        return;

    m_exclusive = exclusive;
    if (GTK_IS_CHECK_MENU_ITEM(m_item))
        g_object_set(m_item, "draw-as-radio", exclusive, NULL);
}

void QGtk3MenuItem::onSelect(GtkMenuItem *, void *data)
{
    QGtk3MenuItem *item = static_cast<QGtk3MenuItem *>(data);
    if (item)
        emit item->hovered();
}

void QGtk3MenuItem::onActivate(GtkMenuItem *, void *data)
{
    QGtk3MenuItem *item = static_cast<QGtk3MenuItem *>(data);
    if (item)
        emit item->activated();
}

void QGtk3MenuItem::onToggle(GtkCheckMenuItem *check, void *data)
{
    QGtk3MenuItem *item = static_cast<QGtk3MenuItem *>(data);
    if (item) {
        bool active = gtk_check_menu_item_get_active(check);
        if (active != item->isChecked()) {
            item->setChecked(active);
            emit item->activated();
        }
    }
}

QGtk3Menu::QGtk3Menu()
{
    m_menu = gtk_menu_new();

    g_signal_connect(m_menu, "show", G_CALLBACK(onShow), this);
    g_signal_connect(m_menu, "hide", G_CALLBACK(onHide), this);
}

QGtk3Menu::~QGtk3Menu()
{
    if (GTK_IS_WIDGET(m_menu))
        gtk_widget_destroy(m_menu);
}

GtkWidget *QGtk3Menu::handle() const
{
    return m_menu;
}

void QGtk3Menu::insertMenuItem(QPlatformMenuItem *item, QPlatformMenuItem *before)
{
    QGtk3MenuItem *gitem = static_cast<QGtk3MenuItem *>(item);
    if (!gitem || m_items.contains(gitem))
        return;

    GtkWidget *handle = gitem->create();
    int index = m_items.indexOf(static_cast<QGtk3MenuItem *>(before));
    if (index < 0)
        index = m_items.count();
    m_items.insert(index, gitem);
    gtk_menu_shell_insert(GTK_MENU_SHELL(m_menu), handle, index);
}

void QGtk3Menu::removeMenuItem(QPlatformMenuItem *item)
{
    QGtk3MenuItem *gitem = static_cast<QGtk3MenuItem *>(item);
    if (!gitem || !m_items.removeOne(gitem))
        return;

    GtkWidget *handle = gitem->handle();
    if (handle)
        gtk_container_remove(GTK_CONTAINER(m_menu), handle);
}

void QGtk3Menu::syncMenuItem(QPlatformMenuItem *item)
{
    QGtk3MenuItem *gitem = static_cast<QGtk3MenuItem *>(item);
    int index = m_items.indexOf(gitem);
    if (index == -1 || !gitem->isInvalid())
        return;

    GtkWidget *handle = gitem->create();
    if (handle)
        gtk_menu_shell_insert(GTK_MENU_SHELL(m_menu), handle, index);
}

void QGtk3Menu::syncSeparatorsCollapsible(bool enable)
{
    Q_UNUSED(enable);
}

void QGtk3Menu::setEnabled(bool enabled)
{
    gtk_widget_set_sensitive(m_menu, enabled);
}

void QGtk3Menu::setVisible(bool visible)
{
    gtk_widget_set_visible(m_menu, visible);
}

static void qt_gtk_menu_position_func(GtkMenu *, gint *x, gint *y, gboolean *push_in, gpointer data)
{
    QGtk3Menu *menu = static_cast<QGtk3Menu *>(data);
    QPoint targetPos = menu->targetPos();
#if GTK_CHECK_VERSION(3, 10, 0)
    targetPos /= gtk_widget_get_scale_factor(menu->handle());
#endif
    *x = targetPos.x();
    *y = targetPos.y();
    *push_in = true;
}

QPoint QGtk3Menu::targetPos() const
{
    return m_targetPos;
}

void QGtk3Menu::showPopup(const QWindow *parentWindow, const QRect &targetRect, const QPlatformMenuItem *item)
{
    const QGtk3MenuItem *menuItem = static_cast<const QGtk3MenuItem *>(item);
    if (menuItem)
        gtk_menu_shell_select_item(GTK_MENU_SHELL(m_menu), menuItem->handle());

    m_targetPos = QPoint(targetRect.x(), targetRect.y() + targetRect.height());

    QPlatformWindow *pw = parentWindow ? parentWindow->handle() : nullptr;
    if (pw)
        m_targetPos = pw->mapToGlobal(m_targetPos);

    gtk_menu_popup(GTK_MENU(m_menu), NULL, NULL, qt_gtk_menu_position_func, this, 0, gtk_get_current_event_time());
}

void QGtk3Menu::dismiss()
{
    gtk_menu_popdown(GTK_MENU(m_menu));
}

QPlatformMenuItem *QGtk3Menu::menuItemAt(int position) const
{
    return m_items.value(position);
}

QPlatformMenuItem *QGtk3Menu::menuItemForTag(quintptr tag) const
{
    for (QGtk3MenuItem *item : m_items) {
        if (item->tag() == tag)
            return item;
    }
    return nullptr;
}

QPlatformMenuItem *QGtk3Menu::createMenuItem() const
{
    return new QGtk3MenuItem;
}

QPlatformMenu *QGtk3Menu::createSubMenu() const
{
    return new QGtk3Menu;
}

void QGtk3Menu::onShow(GtkWidget *, void *data)
{
    QGtk3Menu *menu = static_cast<QGtk3Menu *>(data);
    if (menu)
        emit menu->aboutToShow();
}

void QGtk3Menu::onHide(GtkWidget *, void *data)
{
    QGtk3Menu *menu = static_cast<QGtk3Menu *>(data);
    if (menu)
        emit menu->aboutToHide();
}

QT_END_NAMESPACE
