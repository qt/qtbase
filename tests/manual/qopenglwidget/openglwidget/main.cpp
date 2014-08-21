/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
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
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "openglwidget.h"
#include <QApplication>
#include <QPushButton>
#include <QMdiArea>
#include <QLCDNumber>
#include <QTimer>
#include <QSurfaceFormat>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QSurfaceFormat format;
    if (QCoreApplication::arguments().contains(QLatin1String("--multisample")))
        format.setSamples(4);
    if (QCoreApplication::arguments().contains(QLatin1String("--coreprofile"))) {
        format.setVersion(3, 2);
        format.setProfile(QSurfaceFormat::CoreProfile);
    }
    qDebug() << "Requesting" << format;

    QMdiArea w;
    w.resize(400,400);

    OpenGLWidget *glw = new OpenGLWidget;
    glw->setFormat(format);
    w.addSubWindow(glw);
    glw->setMinimumSize(100,100);

    OpenGLWidget *glw2 = new OpenGLWidget;
    glw2->setFormat(format);
    glw2->setMinimumSize(100,100);
    w.addSubWindow(glw2);

    QLCDNumber *lcd = new QLCDNumber;
    lcd->display(1337);
    lcd->setMinimumSize(300,100);
    w.addSubWindow(lcd);

    w.show();

    if (glw->isValid())
        qDebug() << "Got" << glw->format();

    return a.exec();
}
