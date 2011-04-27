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
#include <QtDebug>

#include "shower.h"
#include "qengines.h"

static void usage()
{
    qDebug()<<"shower <-engine engineName> file";
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QString engine = "Raster";
    QString file;
    for (int i = 1; i < argc; ++i) {
        QString opt = argv[i];
        if (opt == "-engine") {
            ++i;
            engine = QString(argv[i]);
        } else if (opt.startsWith('-')) {
            qDebug()<<"Unsupported option "<<opt;
        } else
            file = QString(argv[i]);
    }

    bool engineExists = false;
    QStringList engineNames;
    foreach(QEngine *qengine, QtEngines::self()->engines()) {
        if (qengine->name() == engine) {
            engineExists = true;
        }
        engineNames.append(qengine->name());
    }

    if (file.isEmpty() || engine.isEmpty()) {
        usage();
        return 1;
    }

    if (!engineExists) {
        qDebug()<<"Engine "<<engine<<" doesn't exist!\n"
                <<"Available engines: "<<engineNames;
        usage();
        return 1;
    }
    if (!QFile::exists(file)) {
        qDebug()<<"Specified file "<<file<<" doesn't exist!";
        return 1;
    }

    qDebug()<<"Using engine: "<<engine;
    Shower shower(file, engine);
    shower.show();

    return app.exec();
}
