// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TESTOUTPUTFORWARDER_H
#define TESTOUTPUTFORWARDER_H

#include "hdc.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>
#include <QtCore/qxpfunctional.h>
#include <QtCore/private/qexpected_p.h>

QT_BEGIN_NAMESPACE

class TestOutputForwarder
{
public:
    TestOutputForwarder(Hdc hdc, QString devicePath, QString localCopyPath);

    q23::expected<QByteArray, QString> forwardAvailable();
    bool forwardRemaining(qxp::function_ref<bool()> isInterrupted);

private:
    void forward(const QByteArray &output);
    void reportUnreadOutput(const QString &failure) const;

    Hdc m_hdc;
    QString m_devicePath;
    QString m_localCopyPath;
    qint64 m_forwardedBytes = 0;
};

QT_END_NAMESPACE

#endif // TESTOUTPUTFORWARDER_H
