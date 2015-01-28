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

#include <qtest.h>
#include <QtCore>

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
