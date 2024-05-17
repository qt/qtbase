// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
#include <QTest>

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
    qCDebug(cat, "foo");

    // qWarning still works, though
    QTest::ignoreMessage(QtWarningMsg, "bar");
    QTest::ignoreMessage(QtWarningMsg, "custom-bar");
    qWarning() << "bar";
    qCWarning(cat) << "custom-bar";
}

void tst_QNoDebug::streaming() const
{
    QDateTime dt(QDate(1,2,3),QTime(4,5,6));
    const QByteArray debugString = dt.toString(u"yyyy-MM-dd HH:mm:ss.zzz t").toLocal8Bit();
    const QByteArray message = "QDateTime(" + debugString + " Qt::LocalTime)";
    QTest::ignoreMessage(QtWarningMsg, message.constData());
    qWarning() << dt;
}

QTEST_MAIN(tst_QNoDebug);
#include "tst_qnodebug.moc"
