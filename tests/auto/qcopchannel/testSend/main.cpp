/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QApplication>
#include <QByteArray>
#include <QCopChannel>
#include <stdio.h>
#include <QStringList>

int main(int argc, char** argv)
{
#ifdef Q_WS_QWS
    QApplication app(argc, argv);
    QStringList args = app.arguments();
    Q_ASSERT(args.count() ==  3 || args.count() == 4);
    QString channelName = args.at(1);
    QString msg = args.at(2);
    QByteArray data;
    if(args.count()==4)
	data = QByteArray(args.at(3).toAscii());
    QCopChannel::send(channelName, msg, data);
    QCopChannel::flush();
    fprintf(stdout,"done");
    fflush(stdout);
#endif
    return 0;
}
