// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef PROPERTY_H
#define PROPERTY_H

#include "library/proitems.h"
#include "propertyprinter.h"

#include <qglobal.h>
#include <qstring.h>
#include <qhash.h>

QT_BEGIN_NAMESPACE

class QSettings;

class QMakeProperty final
{
    QSettings *settings;
    void initSettings();

    QHash<ProKey, ProString> m_values;

public:
    QMakeProperty();
    ~QMakeProperty();

    void reload();

    bool hasValue(const ProKey &);
    ProString value(const ProKey &);

    void setValue(QString, const QString &);
    void remove(const QString &);

    int queryProperty(const QStringList &optionProperties = QStringList(),
                      const PropertyPrinter &printer = qmakePropertyPrinter);
    int setProperty(const QStringList &optionProperties);
    void unsetProperty(const QStringList &optionProperties);
};

QT_END_NAMESPACE

#endif // PROPERTY_H
