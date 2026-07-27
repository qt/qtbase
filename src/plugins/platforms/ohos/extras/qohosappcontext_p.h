// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSAPPCONTEXT_P_H
#define QOHOSAPPCONTEXT_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtHarmonyExtras/private/qohosappbundleinfo_p.h>
#include <QtHarmonyExtras/private/qohoswant_p.h>
#include <QtHarmonyExtras/private/qtharmonyextrasglobal_p.h>
#include <functional>
#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QtHarmonyExtras {

class Q_HARMONYEXTRAS_EXPORT AppContext : public QObject
{
    Q_OBJECT

public:
    static AppContext *instance();

    static bool isNoUiChildMode();

    static void startNoUiChildProcess(const QString &libraryName, const QStringList &args);

    static std::shared_ptr<WantInfo> appLaunchWantInfo();

    virtual bool hasSerialPortAccessRight(const QString &portName) const = 0;
    virtual void requestSerialPortAccessRightIfNeeded(
        const QString &portName, QObject *context,
        std::function<void(std::shared_ptr<QObject>)> callback) = 0;

    virtual std::shared_ptr<BundleInfo> bundleInfo() const = 0;

    Q_NORETURN virtual void restartApp(const std::optional<Want> &want) = 0;

    virtual double fontSizeScale() const = 0;

Q_SIGNALS:
    void fontSizeScaleChanged(double fontSizeScale);

protected:
    AppContext();
    ~AppContext() override;

    Q_DISABLE_COPY(AppContext)
};

}

QT_END_NAMESPACE

#endif
