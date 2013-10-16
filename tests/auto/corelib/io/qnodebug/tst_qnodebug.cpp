/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
    QString debugString = dt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz t"))
                        + QStringLiteral(" Qt::LocalTime");
    QTest::ignoreMessage(QtWarningMsg, qPrintable(QString::fromLatin1("QDateTime(\"%1\")").arg(debugString)));
    qWarning() << dt;
}

QTEST_MAIN(tst_QNoDebug);
#include "tst_qnodebug.moc"
