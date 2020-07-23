/****************************************************************************
**
** Copyright (C) 2017 Intel Corporation.
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

#include <QCoreApplication>
#include <QOperatingSystemVersion>
#include <QSysInfo>

#include <stdio.h>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    printf("QSysInfo::WordSize = %d\n", QSysInfo::WordSize);
    printf("QSysInfo::ByteOrder = QSysInfo::%sEndian\n",
           QSysInfo::ByteOrder == QSysInfo::LittleEndian ? "Little" : "Big");
    printf("QSysInfo::buildCpuArchitecture() = %s\n", qPrintable(QSysInfo::buildCpuArchitecture()));
    printf("QSysInfo::currentCpuArchitecture() = %s\n", qPrintable(QSysInfo::currentCpuArchitecture()));
    printf("QSysInfo::buildAbi() = %s\n", qPrintable(QSysInfo::buildAbi()));
    printf("QSysInfo::kernelType() = %s\n", qPrintable(QSysInfo::kernelType()));
    printf("QSysInfo::kernelVersion() = %s\n", qPrintable(QSysInfo::kernelVersion()));
    printf("QSysInfo::productType() = %s\n", qPrintable(QSysInfo::productType()));
    printf("QSysInfo::productVersion() = %s\n", qPrintable(QSysInfo::productVersion()));
    printf("QSysInfo::prettyProductName() = %s\n", qPrintable(QSysInfo::prettyProductName()));
    printf("QSysInfo::machineHostName() = %s\n", qPrintable(QSysInfo::machineHostName()));
    printf("QSysInfo::machineUniqueId() = %s\n", QSysInfo::machineUniqueId().constData());
    printf("QSysInfo::bootUniqueId() = %s\n", qPrintable(QSysInfo::bootUniqueId()));

    const auto osv = QOperatingSystemVersion::current();
    printf("QOperatingSystemVersion::current() = %s %d.%d.%d\n",
        qPrintable(osv.name()),
        osv.majorVersion(),
        osv.minorVersion(),
        osv.microVersion());

    return 0;
}
