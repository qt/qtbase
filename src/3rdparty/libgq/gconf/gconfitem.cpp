/* * This file is part of libgq *
 *
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
 * All rights reserved.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QVariant>
#include <QtDebug>

#include "gconfitem.h"

#include <glib.h>
#include <gconf/gconf-value.h>
#include <gconf/gconf-client.h>

struct GConfItemPrivate {
    QString key;
    QVariant value;
    guint notify_id;

    static void notify_trampoline(GConfClient*, guint, GConfEntry *, gpointer);
};

/* We get the default client and never release it, on purpose, to
   avoid disconnecting from the GConf daemon when a program happens to
   not have any GConfItems for short periods of time.
 */
static GConfClient *
get_gconf_client ()
{
  static bool initialized = false;
  static GConfClient *client;

  if (initialized)
    return client;

  g_type_init ();
  client = gconf_client_get_default();
  initialized = true;
    
  return client;
}

/* Sometimes I like being too clever...
 */
#define withClient(c) for (GConfClient *c = get_gconf_client (); c; c = NULL)

static QByteArray convertKey (QString key)
{
    if (key.startsWith('/'))
        return key.toUtf8();
    else
    {
        qWarning() << "Using dot-separated key names with GConfItem is deprecated.";
        qWarning() << "Please use" << '/' + key.replace('.', '/') << "instead of" << key;
        return '/' + key.replace('.', '/').toUtf8();
    }
}

static QString convertKey(const char *key)
{
    return QString::fromUtf8(key);
}

static QVariant convertValue(GConfValue *src)
{
    if (!src) {
        return QVariant();
    } else {
        switch (src->type) {
        case GCONF_VALUE_INVALID:
            return QVariant(QVariant::Invalid);
        case GCONF_VALUE_BOOL:
            return QVariant((bool)gconf_value_get_bool(src));
        case GCONF_VALUE_INT:
            return QVariant(gconf_value_get_int(src));
        case GCONF_VALUE_FLOAT:
            return QVariant(gconf_value_get_float(src));
        case GCONF_VALUE_STRING:
            return QVariant(QString::fromUtf8(gconf_value_get_string(src)));
        case GCONF_VALUE_LIST:
            switch (gconf_value_get_list_type(src)) {
            case GCONF_VALUE_STRING:
                {
                    QStringList result;
                    for (GSList *elts = gconf_value_get_list(src); elts; elts = elts->next)
                        result.append(QString::fromUtf8(gconf_value_get_string((GConfValue *)elts->data)));
                    return QVariant(result);
                }
            default:
                {
                    QList<QVariant> result;
                    for (GSList *elts = gconf_value_get_list(src); elts; elts = elts->next)
                        result.append(convertValue((GConfValue *)elts->data));
                    return QVariant(result);
                }
            }
        case GCONF_VALUE_SCHEMA:
        default:
            return QVariant();
        }
    }
}

static GConfValue *convertString(const QString &str)
{
    GConfValue *v = gconf_value_new (GCONF_VALUE_STRING);
    gconf_value_set_string (v, str.toUtf8().data());
    return v;
}

static GConfValueType primitiveType (const QVariant &elt)
{
    switch(elt.type()) {
    case QVariant::String:
        return GCONF_VALUE_STRING;
    case QVariant::Int:
        return GCONF_VALUE_INT;
    case QVariant::Double:
        return GCONF_VALUE_FLOAT;
    case QVariant::Bool:
        return GCONF_VALUE_BOOL;
    default:
        return GCONF_VALUE_INVALID;
    }
}

static GConfValueType uniformType(const QList<QVariant> &list)
{
    GConfValueType result = GCONF_VALUE_INVALID;

    foreach (const QVariant &elt, list) {
        GConfValueType elt_type = primitiveType (elt);

        if (elt_type == GCONF_VALUE_INVALID)
            return GCONF_VALUE_INVALID;

        if (result == GCONF_VALUE_INVALID)
            result = elt_type;
        else if (result != elt_type)
            return GCONF_VALUE_INVALID;
    }

    if (result == GCONF_VALUE_INVALID)
        return GCONF_VALUE_STRING;  // empty list.
    else
        return result;
}

static int convertValue(const QVariant &src, GConfValue **valp)
{
    GConfValue *v;

    switch(src.type()) {
    case QVariant::Invalid:
        v = NULL;
        break;
    case QVariant::Bool:
        v = gconf_value_new (GCONF_VALUE_BOOL);
        gconf_value_set_bool (v, src.toBool());
        break;
    case QVariant::Int:
        v = gconf_value_new (GCONF_VALUE_INT);
        gconf_value_set_int (v, src.toInt());
        break;
    case QVariant::Double:
        v = gconf_value_new (GCONF_VALUE_FLOAT);
        gconf_value_set_float (v, src.toDouble());
        break;
    case QVariant::String:
        v = convertString(src.toString());
        break;
    case QVariant::StringList:
        {
            GSList *elts = NULL;
            v = gconf_value_new(GCONF_VALUE_LIST);
            gconf_value_set_list_type(v, GCONF_VALUE_STRING);
            foreach (const QString &str, src.toStringList())
                elts = g_slist_prepend(elts, convertString(str));
            gconf_value_set_list_nocopy(v, g_slist_reverse(elts));
            break;
        }
    case QVariant::List:
        {
            GConfValueType elt_type = uniformType(src.toList());
            if (elt_type == GCONF_VALUE_INVALID)
                v = NULL;
            else
            {
                GSList *elts = NULL;
                v = gconf_value_new(GCONF_VALUE_LIST);
                gconf_value_set_list_type(v, elt_type);
                foreach (const QVariant &elt, src.toList())
                {
                    GConfValue *val = NULL;
                    convertValue(elt, &val);  // guaranteed to succeed.
                    elts = g_slist_prepend(elts, val);
                }
                gconf_value_set_list_nocopy(v, g_slist_reverse(elts));
            }
            break;
        }
    default:
        return 0;
    }

    *valp = v;
    return 1;
}

void GConfItemPrivate::notify_trampoline (GConfClient*,
                                             guint,
                                             GConfEntry *,
                                             gpointer data)
{
    GConfItem *item = (GConfItem *)data;
    item->update_value (true);
}

void GConfItem::update_value (bool emit_signal)
{
    QVariant new_value;

    withClient(client) {
        GError *error = NULL;
        QByteArray k = convertKey(priv->key);
        GConfValue *v = gconf_client_get(client, k.data(), &error);

        if (error) {
            qWarning() << error->message;
            g_error_free (error);
            new_value = priv->value;
        } else {
            new_value = convertValue(v);
            if (v)
                gconf_value_free(v);
        }
    }

    if (new_value != priv->value) {
        priv->value = new_value;
        if (emit_signal)
            emit valueChanged();
    }
}

QString GConfItem::key() const
{
    return priv->key;
}

QVariant GConfItem::value() const
{
    return priv->value;
}

QVariant GConfItem::value(const QVariant &def) const
{
    if (priv->value.isNull())
        return def;
    else
        return priv->value;
}

void GConfItem::set(const QVariant &val)
{
    withClient(client) {
        QByteArray k = convertKey(priv->key);
        GConfValue *v;
        if (convertValue(val, &v)) {
            GError *error = NULL;

            if (v) {
                gconf_client_set(client, k.data(), v, &error);
                gconf_value_free(v);
            } else {
                gconf_client_unset(client, k.data(), &error);
            }

            if (error) {
                qWarning() << error->message;
                g_error_free(error);
            } else if (priv->value != val) {
                priv->value = val;
                emit valueChanged();
            }

        } else
            qWarning() << "Can't store a" << val.typeName();
    }
}

void GConfItem::unset() {
    set(QVariant());
}

QList<QString> GConfItem::listDirs() const
{
    QList<QString> children;

    withClient(client) {
        QByteArray k = convertKey(priv->key);
        GSList *dirs = gconf_client_all_dirs(client, k.data(), NULL);
        for (GSList *d = dirs; d; d = d->next) {
            children.append(convertKey((char *)d->data));
            g_free (d->data);
        }
        g_slist_free (dirs);
    }

    return children;
}

QList<QString> GConfItem::listEntries() const
{
    QList<QString> children;

    withClient(client) {
        QByteArray k = convertKey(priv->key);
        GSList *entries = gconf_client_all_entries(client, k.data(), NULL);
        for (GSList *e = entries; e; e = e->next) {
            children.append(convertKey(((GConfEntry *)e->data)->key));
            gconf_entry_free ((GConfEntry *)e->data);
        }
        g_slist_free (entries);
    }

    return children;
}

GConfItem::GConfItem(const QString &key, QObject *parent)
    : QObject (parent)
{
    priv = new GConfItemPrivate;
    priv->key = key;
    priv->notify_id = 0;
    withClient(client) {
        update_value (false);
        QByteArray k = convertKey(priv->key);
        gconf_client_add_dir (client, k.data(), GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);
        priv->notify_id = gconf_client_notify_add (client, k.data(),
                                                   GConfItemPrivate::notify_trampoline, this,
                                                   NULL, NULL);
    }
}

GConfItem::~GConfItem()
{
    withClient(client) {
        QByteArray k = convertKey(priv->key);
        if (priv->notify_id)
          gconf_client_notify_remove (client, priv->notify_id);
        gconf_client_remove_dir (client, k.data(), NULL);
    }
    delete priv;
}
