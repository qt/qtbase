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

#include <qtest.h>
#include <QtCore>

//TESTED_FILES=

QThreadStorage<int *> dummy[8];

QThreadStorage<QString *> tls1;

class tst_QThreadStorage : public QObject
{
    Q_OBJECT

public:
    tst_QThreadStorage();
    virtual ~tst_QThreadStorage();

public slots:
    void init();
    void cleanup();

private slots:
    void construct();
    void get();
    void set();
};

tst_QThreadStorage::tst_QThreadStorage()
{
}

tst_QThreadStorage::~tst_QThreadStorage()
{
}

void tst_QThreadStorage::init()
{
    dummy[1].setLocalData(new int(5));
    dummy[2].setLocalData(new int(4));
    dummy[3].setLocalData(new int(3));
    tls1.setLocalData(new QString());
}

void tst_QThreadStorage::cleanup()
{
}

void tst_QThreadStorage::construct()
{
    QBENCHMARK {
        QThreadStorage<int *> ts;
    }
}


void tst_QThreadStorage::get()
{
    QThreadStorage<int *> ts;
    ts.setLocalData(new int(45));

    int count = 0;
    QBENCHMARK {
        int *i = ts.localData();
        count += *i;
    }
    ts.setLocalData(0);
}

void tst_QThreadStorage::set()
{
    QThreadStorage<int *> ts;

    int count = 0;
    QBENCHMARK {
        ts.setLocalData(new int(count));
        count++;
    }
    ts.setLocalData(0);
}


QTEST_MAIN(tst_QThreadStorage)
#include "tst_qthreadstorage.moc"
