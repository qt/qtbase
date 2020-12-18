/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qcoreapplication.h>
#include <QtCore/qmetaobject.h>
#include <QtNetwork/qnetworkinformation.h>

QByteArray nameOfEnumValue(QNetworkInformation::Reachability reachability)
{
    return QMetaEnum::fromType<QNetworkInformation::Reachability>().valueToKey(int(reachability));
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QTextStream writer(stdout);

    if (!QNetworkInformation::load(QNetworkInformation::Feature::Reachability)) {
        qWarning("Failed to load any backend");
        writer << "Backends available: " << QNetworkInformation::availableBackends().join(", ") << '\n';
        return -1;
    }
    QNetworkInformation *info = QNetworkInformation::instance();
    writer << "Backend loaded: " << info->backendName() << '\n';
    writer << "Now you can make changes to the current network connection. Qt should see the "
              "changes and notify about it.\n";
    QObject::connect(info, &QNetworkInformation::reachabilityChanged,
                     [&writer](QNetworkInformation::Reachability newStatus) {
                         writer << "Updated: " << nameOfEnumValue(newStatus) << '\n';
                         writer.flush();
                     });

    writer << "Initial: " << nameOfEnumValue(info->reachability()) << '\n';
    writer.flush();

    return app.exec();
}
