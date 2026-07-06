// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSAPPCONTEXT_H
#define QOHOSAPPCONTEXT_H

#include <QtCore/qobject.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtOhosAppKit/qohosappbundleinfo.h>
#include <QtOhosAppKit/qohoswant.h>
#include <QtOhosAppKit/qtohosappkitglobal.h>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

class Q_OHOSAPPKIT_EXPORT QOhosAppContext : public QObject
{
    Q_OBJECT

public:
    static QOhosAppContext *instance();

    static void startNoUiChildProcess(QString libraryName, QStringList args);

    static QOhosWant getAppLaunchWant();
    static QSharedPointer<QOhosWantInfo> getAppLaunchWantInfo();

    virtual bool hasSerialPortAccessRight(const QString &portName) const = 0;
    virtual void requestSerialPortAccessRightIfNeeded(const QString &portName) = 0;

    virtual QSharedPointer<QOhosBundleInfo> getBundleInfo() const = 0;

    Q_NORETURN virtual void restartApp() = 0;

    Q_NORETURN virtual void restartApp(const QOhosWant &want) = 0;

Q_SIGNALS:
    void serialPortAccessRightResponseReceived(const QString &portName, QSharedPointer<QObject> serialPortAccessRightContext);

protected:
    QOhosAppContext();
    ~QOhosAppContext() override;

    Q_DISABLE_COPY(QOhosAppContext)
};

}

QT_END_NAMESPACE

#endif
