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
#include <QDebug>
#include <QIODevice>
#include <QFile>
#include <QString>
#include <QTemporaryFile>
#include <qtest.h>


class tst_qtemporaryfile : public QObject
{
    Q_OBJECT
private slots:
    void openclose_data();
    void openclose();
    void readwrite_data() { openclose_data(); }
    void readwrite();

private:
};

void tst_qtemporaryfile::openclose_data()
{
    QTest::addColumn<qint64>("amount");
    QTest::newRow("100")   << qint64(100);
    QTest::newRow("1000")  << qint64(1000);
    QTest::newRow("10000") << qint64(10000);
}

void tst_qtemporaryfile::openclose()
{
    QFETCH(qint64, amount);

    QBENCHMARK {
        for (qint64 i = 0; i < amount; ++i) {
            QTemporaryFile file;
            file.open();
            file.close();
        }
    }
}

void tst_qtemporaryfile::readwrite()
{
    QFETCH(qint64, amount);

    const int dataSize = 4096;
    QByteArray data;
    data.fill('a', dataSize);
    QBENCHMARK {
        for (qint64 i = 0; i < amount; ++i) {
            QTemporaryFile file;
            file.open();
            file.write(data);
            file.seek(0);
            file.read(dataSize);
            file.close();
        }
    }
}

QTEST_MAIN(tst_qtemporaryfile)

#include "main.moc"
