/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QNETWORKINFORMATION_H
#define QNETWORKINFORMATION_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qstringview.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class QNetworkInformationBackend;
class QNetworkInformationPrivate;
struct QNetworkInformationDeleter;
class Q_NETWORK_EXPORT QNetworkInformation : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QNetworkInformation)
    Q_PROPERTY(Reachability reachability READ reachability NOTIFY reachabilityChanged)
public:
    enum class Reachability {
        Unknown,
        Disconnected,
        Local,
        Site,
        Online,
    };
    Q_ENUM(Reachability)

    enum class Feature {
        Reachability = 0x1,
    };
    Q_DECLARE_FLAGS(Features, Feature)
    Q_FLAG(Features)

    Reachability reachability() const;

    QString backendName() const;

    bool supports(Features features) const;

    static bool load(QStringView backend);
    static bool load(Features features);
    static QStringList availableBackends();
    static QNetworkInformation *instance();

Q_SIGNALS:
    void reachabilityChanged(Reachability newReachability);

private:
    friend struct QNetworkInformationDeleter;
    friend class QNetworkInformationPrivate;
    QNetworkInformation(QNetworkInformationBackend *backend);
    ~QNetworkInformation() override;

    Q_DISABLE_COPY_MOVE(QNetworkInformation)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QNetworkInformation::Features)

QT_END_NAMESPACE

#endif
