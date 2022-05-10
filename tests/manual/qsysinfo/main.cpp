// Copyright (C) 2017 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
