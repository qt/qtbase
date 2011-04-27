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


#include "uiloader.h"

#include <QtTest/QtTest>
#include <QApplication>
#include <QDir>

#ifdef Q_OS_SYMBIAN
#define SRCDIR ""
#endif

class uiLoaderAutotest: public QObject
{

Q_OBJECT

public slots:
    void init();

private slots:
    void imageDiffTest();
    
private:
    QString currentDir;
        
};



void uiLoaderAutotest::init()
{
    currentDir = QDir::currentPath();
#ifndef Q_OS_IRIX
    QDir::setCurrent(QString(SRCDIR) + QString("/.."));
#endif
}

void uiLoaderAutotest::imageDiffTest()
{
    //QApplication app(argc, argv);

    QString pathToProgram   = currentDir + "/tst_screenshot/tst_screenshot";

#ifdef Q_WS_MAC
    pathToProgram += ".app/Contents/MacOS/tst_screenshot";
#endif

#ifdef Q_WS_WIN
    pathToProgram += ".exe";
#endif
    uiLoader wrapper(pathToProgram);
    QString errorMessage;
    switch(wrapper.runAutoTests(&errorMessage)) {
        case uiLoader::TestRunDone:
        break;
        case uiLoader::TestConfigError:
        QVERIFY2(false, qPrintable(errorMessage));
        break;
        case uiLoader::TestNoConfig:
        QSKIP(qPrintable(errorMessage), SkipAll);
        break;
    }
}

QTEST_MAIN(uiLoaderAutotest)
#include "tst_uiloader.moc"
