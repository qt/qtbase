// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSOPERATIONSTATUS_H
#define QOHOSOPERATIONSTATUS_H

#include <QtCore/qglobal.h>
#include <QtOhosAppKit/qtohosappkitglobal.h>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

class Q_OHOSAPPKIT_EXPORT OperationStatus
{
public:
    virtual ~OperationStatus();

    virtual bool success() const = 0;

protected:
    OperationStatus();

private:
    Q_DISABLE_COPY(OperationStatus)
};

}

QT_END_NAMESPACE

#endif // QOHOSOPERATIONSTATUS_H
