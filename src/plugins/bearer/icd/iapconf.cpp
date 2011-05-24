/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/


#include <stdlib.h>
#include <string.h>
#include <conn_settings.h>

#include "iapconf.h"

#define QSTRING_TO_CONST_CSTR(str) \
    str.toUtf8().constData()

namespace Maemo {

class IAPConfPrivate {
public:
    ConnSettings *settings;

    ConnSettingsValue *variantToValue(const QVariant &variant);
    QVariant valueToVariant(ConnSettingsValue *value);
};

ConnSettingsValue *IAPConfPrivate::variantToValue(const QVariant &variant)
{
    // Convert variant to ConnSettingsValue
    ConnSettingsValue *value = conn_settings_value_new();
    if (value == 0) {
        qWarning("IAPConf: Unable to create new ConnSettingsValue");
        return 0;
    }

    switch(variant.type()) {
        
    case QVariant::Invalid:
        value->type = CONN_SETTINGS_VALUE_INVALID;
        break;
    
    case QVariant::String: {
        char *valueStr = strdup(QSTRING_TO_CONST_CSTR(variant.toString()));
        value->type = CONN_SETTINGS_VALUE_STRING;
        value->value.string_val = valueStr;
        break;
    }
    
    case QVariant::Int:
        value->type = CONN_SETTINGS_VALUE_INT;
        value->value.int_val = variant.toInt();
        break;
    
    case QMetaType::Float:
    case QVariant::Double:
        value->type = CONN_SETTINGS_VALUE_DOUBLE;
        value->value.double_val = variant.toDouble();
        break;
    
    case QVariant::Bool:
        value->type = CONN_SETTINGS_VALUE_BOOL;
        value->value.bool_val = variant.toBool() ? 1 : 0;
        break;
    
    case QVariant::ByteArray: {
        QByteArray array = variant.toByteArray();
        value->type = CONN_SETTINGS_VALUE_BYTE_ARRAY;
        value->value.byte_array.len = array.size();
        value->value.byte_array.val = (unsigned char *)malloc(array.size());
        memcpy(value->value.byte_array.val, array.constData(), array.size());
        break;
    }
    
    case QVariant::List: {
        QVariantList list = variant.toList();
        ConnSettingsValue **list_val = (ConnSettingsValue **)malloc(
            (list.size() + 1) * sizeof(ConnSettingsValue *));

        for (int idx = 0; idx < list.size(); idx++) {
            list_val[idx] = variantToValue(list.at(idx));
        }
        list_val[list.size()] = 0;

        value->type = CONN_SETTINGS_VALUE_LIST;
        value->value.list_val = list_val;
        break;
    }
    
    default:
        qWarning("IAPConf: Can not handle QVariant of type %d",
                 variant.type());
        conn_settings_value_destroy(value);
        return 0;
    }

    return value;
}

QVariant IAPConfPrivate::valueToVariant(ConnSettingsValue *value)
{
    if (value == 0 || value->type == CONN_SETTINGS_VALUE_INVALID) {
        return QVariant();
    }

    switch(value->type) {
    
    case CONN_SETTINGS_VALUE_BOOL:
        return QVariant(value->value.bool_val ? true : false);
    
    case CONN_SETTINGS_VALUE_STRING:
        return QVariant(QString(value->value.string_val));
    
    case CONN_SETTINGS_VALUE_DOUBLE:
        return QVariant(value->value.double_val);
    
    case CONN_SETTINGS_VALUE_INT:
        return QVariant(value->value.int_val);
    
    case CONN_SETTINGS_VALUE_LIST: {
        // At least with GConf backend connsettings returns byte array as list
        // of ints, first check for that case
        if (value->value.list_val && value->value.list_val[0]) {
            bool canBeConvertedToByteArray = true;
            for (int idx = 0; value->value.list_val[idx]; idx++) {
                ConnSettingsValue *val = value->value.list_val[idx];
                if (val->type != CONN_SETTINGS_VALUE_INT
                     || val->value.int_val > 255
                     || val->value.int_val < 0) {
                    canBeConvertedToByteArray = false;
                    break;
                }
            }

            if (canBeConvertedToByteArray) {
                QByteArray array;
                for (int idx = 0; value->value.list_val[idx]; idx++) {
                    array.append(value->value.list_val[idx]->value.int_val);
                }
                return array;
            }

	    // Create normal list
	    QVariantList list;
	    for (int idx = 0; value->value.list_val[idx]; idx++) {
	      list.append(valueToVariant(value->value.list_val[idx]));
	    }
	    return list;
        }
    }
    
    case CONN_SETTINGS_VALUE_BYTE_ARRAY:
        return QByteArray::fromRawData((char *)value->value.byte_array.val, 
                                       value->value.byte_array.len);
    
    default:
        return QVariant();
    }
}

// Public class implementation

IAPConf::IAPConf(const QString &iap_id)
    : d_ptr(new IAPConfPrivate)
{
    d_ptr->settings = conn_settings_open(CONN_SETTINGS_CONNECTION,
                                         QSTRING_TO_CONST_CSTR(iap_id));
    if (d_ptr->settings == 0) {
        qWarning("IAPConf: Unable to open ConnSettings for %s", 
                 QSTRING_TO_CONST_CSTR(iap_id));
    }
}

IAPConf::~IAPConf()
{
    conn_settings_close(d_ptr->settings);
    delete d_ptr;
}


QVariant IAPConf::value(const QString& key) const
{
    ConnSettingsValue *val = conn_settings_get(d_ptr->settings,
                                               QSTRING_TO_CONST_CSTR(key));

    QVariant variant = d_ptr->valueToVariant(val);
    conn_settings_value_destroy(val);
    return variant;
}


void IAPConf::getAll(QList<QString> &all_iaps, bool return_path)
{
    Q_UNUSED(return_path); // We don't use return path currently

    // Go through all available connections and add them to the list
    char **ids = conn_settings_list_ids(CONN_SETTINGS_CONNECTION);
    if (ids == 0) {
        // No ids found - nothing to do
        return;
    }

    for (int idx = 0; ids[idx]; idx++) {
        all_iaps.append(QString(ids[idx]));
        free(ids[idx]);
    }
    free(ids);
}


} // namespace Maemo
