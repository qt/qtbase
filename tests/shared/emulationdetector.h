/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#ifndef EMULATIONDETECTOR_H
#define EMULATIONDETECTOR_H

#include <QtCore/qglobal.h>

#if defined(Q_OS_LINUX) && defined(Q_PROCESSOR_ARM)
#define SHOULD_CHECK_ARM_ON_X86

#include <QFileInfo>

#if QT_CONFIG(process) && QT_CONFIG(regularexpression)
#include <QProcess>
#include <QRegularExpression>
#endif

#endif

QT_BEGIN_NAMESPACE

// Helper functions for detecting if running emulated
namespace EmulationDetector {

#ifdef SHOULD_CHECK_ARM_ON_X86
static bool isX86SpecificFileAvailable(void);
static bool isReportedArchitectureX86(void);
#endif

/*
 * Check if we are running Arm binary on x86 machine.
 *
 * Currently this is only able to check on Linux. If not able to
 * detect, return false.
 */
static bool isRunningArmOnX86()
{
#ifdef SHOULD_CHECK_ARM_ON_X86
    if (isX86SpecificFileAvailable())
        return true;

    if (isReportedArchitectureX86())
        return true;
#endif
    return false;
}

#ifdef SHOULD_CHECK_ARM_ON_X86
/*
 * Check if we can find a file that's only available on x86
 */
static bool isX86SpecificFileAvailable()
{
    // MTRR (Memory Type Range Registers) are a feature of the x86 architecture
    // and /proc/mtrr is only present (on Linux) for that family.
    // However, it's an optional kernel feature, so the absence of the file is
    // not sufficient to conclude we're on real hardware.
    QFileInfo mtrr("/proc/mtrr");
    if (mtrr.exists())
        return true;
    return false;
}

/*
 * Check if architecture reported by the OS is x86
 */
static bool isReportedArchitectureX86(void)
{
#if QT_CONFIG(process) && QT_CONFIG(regularexpression)
    QProcess unamer;
    QString machineString;

    // Using syscall "uname" is not possible since that would be captured by
    // QEMU and result would be the architecture being emulated (e.g. armv7l).
    // By using QProcess we get the architecture used by the host.
    unamer.start("uname -a");
    if (!unamer.waitForFinished()) {
        return false;
    }
    machineString = unamer.readAll();

    // Is our current host cpu x86?
    if (machineString.contains(QRegularExpression("i386|i686|x86"))) {
        return true;
    }
#endif

    return false;
}
#endif // SHOULD_CHECK_ARM_ON_X86

} // EmulationDetector namespace

QT_END_NAMESPACE

#endif

