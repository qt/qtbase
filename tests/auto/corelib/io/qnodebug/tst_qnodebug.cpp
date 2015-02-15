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

// This test is for "release" mode, with -DQT_NO_DEBUG -DQT_NO_DEBUG_OUTPUT
#ifndef QT_NO_DEBUG
#define QT_NO_DEBUG
#endif
#ifndef QT_NO_DEBUG_OUTPUT
#define QT_NO_DEBUG_OUTPUT
#endif

#include <QtCore/QtCore>
#include <QtCore/QtDebug>
#include <QtCore/QLoggingCategory>
#include <QtTest/QtTest>

class tst_QNoDebug: public QObject
{
    Q_OBJECT
private slots:
    void noDebugOutput() const;
    void streaming() const;
};

void tst_QNoDebug::noDebugOutput() const
{
    QLoggingCategory cat("custom");
    // should do nothing
    qDebug() << "foo";
    qCDebug(cat) << "foo";

    // qWarning still works, though
    QTest::ignoreMessage(QtWarningMsg, "bar");
    QTest::ignoreMessage(QtWarningMsg, "custom-bar");
    qWarning() << "bar";
    qCWarning(cat) << "custom-bar";
}

void tst_QNoDebug::streaming() const
{
    QDateTime dt(QDate(1,2,3),QTime(4,5,6));
    const QString debugString = dt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz t"));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(QString::fromLatin1("QDateTime(%1 Qt::TimeSpec(LocalTime))").arg(debugString)));
    qWarning() << dt;
}

QTEST_MAIN(tst_QNoDebug);
#include "tst_qnodebug.moc"
