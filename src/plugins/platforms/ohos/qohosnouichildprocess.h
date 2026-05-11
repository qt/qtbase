// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSNOUIPROCESS_H
#define QOHOSNOUIPROCESS_H

#include <QtCore/qjsonobject.h>

QT_BEGIN_NAMESPACE

namespace QtOhos {

QJsonObject readChildProcessSetupData();

void sendChildProcessSetupData(int childPid, QJsonObject setupData);

}

QT_END_NAMESPACE

#endif // QOHOSNOUIPROCESS_H
