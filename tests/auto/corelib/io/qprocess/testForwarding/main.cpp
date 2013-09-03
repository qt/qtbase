/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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
