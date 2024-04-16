// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "calendarplugin.h"

JulianGregorianPlugin::JulianGregorianPlugin()
{
}
//![0]
QCalendar::SystemId JulianGregorianPlugin::loadCalendar(QAnyStringView request)
{
    Q_ASSERT(!request.isEmpty());
    QStringList names = request.toString().split(u';');
    if (names.size() < 1)
        return {};
    QString dateString = names.takeFirst();
    auto date = QDate::fromString(dateString, u"yyyy-MM-dd",
                                  QCalendar(QCalendar::System::Julian));
    if (!date.isValid())
        return {};
    QString primary = names.isEmpty() ?
            QString::fromStdU16String(u"Julian until ") + dateString : names[0];
    auto backend = new JulianGregorianCalendar(date, primary);
    names.emplaceFront(backend->name());
    auto cid = backend->registerCustomBackend(names);
    return cid;
}

JulianGregorianPlugin::~JulianGregorianPlugin()
{
}
//![0]
