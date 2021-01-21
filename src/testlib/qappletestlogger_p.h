/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QAPPLETESTLOGGER_P_H
#define QAPPLETESTLOGGER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtTest/private/qabstracttestlogger_p.h>

#include <QtCore/private/qcore_mac_p.h>

QT_BEGIN_NAMESPACE

#if defined(QT_USE_APPLE_UNIFIED_LOGGING)
class QAppleTestLogger : public QAbstractTestLogger
{
public:
    static bool debugLoggingEnabled();

    QAppleTestLogger();

    void enterTestFunction(const char *function) override;
    void leaveTestFunction() override;

    void addIncident(IncidentTypes type, const char *description,
                     const char *file = 0, int line = 0) override;
    void addMessage(QtMsgType, const QMessageLogContext &,
            const QString &) override;
    void addMessage(MessageTypes type, const QString &message,
                            const char *file = 0, int line = 0) override;

    void addBenchmarkResult(const QBenchmarkResult &result) override
    { Q_UNUSED(result); }

private:
    QString subsystem() const;
    QString testIdentifier() const;
};
#endif

QT_END_NAMESPACE

#endif // QAPPLETESTLOGGER_P_H
