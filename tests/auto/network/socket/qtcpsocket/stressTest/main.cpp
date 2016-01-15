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
#include "Test.h"

#include <QCoreApplication>
#include <QStringList>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QString arg;
    if (app.arguments().size() > 1)
        arg = app.arguments().at(1).toLower().trimmed();

    Test::Type type;
    if (arg == QLatin1String("qt4client"))
        type = Test::Qt4Client;
    else if (arg == QLatin1String("qt4server"))
        type = Test::Qt4Server;
    else {
        qDebug("usage: ./stressTest <qt4client|qt4server>");
        return 0;
    }

    Test test(type);

    return app.exec();
}
