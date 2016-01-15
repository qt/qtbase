/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>

#include <stdlib.h>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (argc < 3)
        return 13;

#ifndef QT_NO_PROCESS
    QProcess process;

    QProcess::ProcessChannelMode mode = (QProcess::ProcessChannelMode)atoi(argv[1]);
    process.setProcessChannelMode(mode);
    if (process.processChannelMode() != mode)
        return 1;

    QProcess::InputChannelMode inmode = (QProcess::InputChannelMode)atoi(argv[2]);
    process.setInputChannelMode(inmode);
    if (process.inputChannelMode() != inmode)
        return 11;

    process.start("testProcessEcho2/testProcessEcho2");

    if (!process.waitForStarted(5000))
        return 2;

    if (inmode == QProcess::ManagedInputChannel && process.write("forwarded") != 9)
        return 3;

    process.closeWriteChannel();
    if (!process.waitForFinished(5000))
        return 4;

    if ((mode == QProcess::ForwardedOutputChannel || mode == QProcess::ForwardedChannels)
            && !process.readAllStandardOutput().isEmpty())
        return 5;
    if ((mode == QProcess::ForwardedErrorChannel || mode == QProcess::ForwardedChannels)
            && !process.readAllStandardError().isEmpty())
        return 6;
#endif
    return 0;
}
