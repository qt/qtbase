/****************************************************************************
**
** Copyright (C) 2011 Robin Burchell <robin+qt@viroteck.net>
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
#include <QtCore>
#include <qtest.h>
#include <qcoreapplication.h>

class QCoreApplicationBenchmark : public QObject
{
Q_OBJECT
private slots:
    void event_posting_benchmark_data();
    void event_posting_benchmark();
};

void QCoreApplicationBenchmark::event_posting_benchmark_data()
{
    QTest::addColumn<int>("size");
    QTest::newRow("50 events") << 50;
    QTest::newRow("100 events") << 100;
    QTest::newRow("200 events") << 200;
    QTest::newRow("1000 events") << 1000;
    QTest::newRow("10000 events") << 10000;
    QTest::newRow("100000 events") << 100000;
    QTest::newRow("1000000 events") << 1000000;
}

void QCoreApplicationBenchmark::event_posting_benchmark()
{
    QFETCH(int, size);

    int type = QEvent::registerEventType();
    QCoreApplication *app = QCoreApplication::instance();

    // benchmark posting & sending events
    QBENCHMARK {
        for (int i = 0; i < size; ++i)
            QCoreApplication::postEvent(app, new QEvent(QEvent::Type(type)));
        QCoreApplication::sendPostedEvents();
    }
}

QTEST_MAIN(QCoreApplicationBenchmark)

#include "main.moc"
