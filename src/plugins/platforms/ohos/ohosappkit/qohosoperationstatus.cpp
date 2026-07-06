// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosoperationstatus.h"
#include <QtOhosAppKit/private/qohosoperationstatus_p.h>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

/*!
    \class QtOhosAppKit::QOhosOperationStatus
    \inmodule QtOhosAppKit
    \since 5.12.12

    \brief The QOhosOperationStatus class provides interface to get an opertaion status.

    An instance of this class is created internally by QtOhosAppKit module. Developer cannot
    create an instance - it is for checking operation status only.
*/

namespace {

class QOhosOperationStatusImpl : public QOhosOperationStatus
{
public:
    QOhosOperationStatusImpl(bool success)
        : QOhosOperationStatus()
        , m_success(success)
    {}

    /*!
        \fn bool QtOhosAppKit::QOhosOperationStatus::success() const

        Returns operations status.

        \return true on success, false otherwise
    */
    bool success() const override
    {
        return m_success;
    }

private:
    bool m_success;
};

}

QOhosOperationStatus::QOhosOperationStatus() = default;
QOhosOperationStatus::~QOhosOperationStatus() = default;

QSharedPointer<QOhosOperationStatus> createOperationStatus(bool status)
{
    return QSharedPointer<QOhosOperationStatusImpl>::create(status);
}

}

QT_END_NAMESPACE
