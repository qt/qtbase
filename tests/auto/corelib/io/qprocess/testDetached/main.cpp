/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QCoreApplication>
#include <QDebug>
#include <QStringList>
#include <QFile>
#include <QDir>

#include <stdio.h>

#if defined(Q_OS_UNIX)
#include <sys/types.h>
#include <unistd.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#endif

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QStringList args = app.arguments();
    if (args.count() != 2) {
        fprintf(stderr, "Usage: testDetached filename.txt\n");
        return 128;
    }

    QFile f(args.at(1));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        fprintf(stderr, "Cannot open %s for writing: %s\n",
                qPrintable(f.fileName()), qPrintable(f.errorString()));
        return 1;
    }

    f.write(QDir::currentPath().toUtf8());
    f.putChar('\n');
#if defined(Q_OS_UNIX)
    f.write(QByteArray::number(quint64(getpid())));
#elif defined(Q_OS_WIN)
    f.write(QByteArray::number(quint64(GetCurrentProcessId())));
#endif
    f.putChar('\n');

    f.close();

    return 0;
}
