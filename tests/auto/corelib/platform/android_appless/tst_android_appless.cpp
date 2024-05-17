// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QTimer>
#include <QGuiApplication>
#include <QWindow>

using namespace Qt::StringLiterals;

class tst_AndroidAppless : public QObject
{
    Q_OBJECT
private slots:
    void app_data();
    void app();

};

#define CREATE_DUMMY_ARGC_ARGV() \
    int argc = 1; \
    char *argv[] = { const_cast<char *>("tst_android_appless") };


void tst_AndroidAppless::app_data()
{
    QTest::addColumn<QString>("displayName");
    QTest::addRow("one") << "The first QGuiApplication instance";
    QTest::addRow("two") << "The second QGuiApplication instance";
}

void tst_AndroidAppless::app()
{
    QFETCH(const QString, displayName);
    CREATE_DUMMY_ARGC_ARGV()

    QGuiApplication app(argc, argv);
    app.setApplicationDisplayName(displayName);

    QWindow window;
    window.show();

    QTimer::singleShot(1000, &app, QGuiApplication::quit);

    window.show();
    app.exec();
}

#undef CREATE_DUMMY_ARGC_ARGV

QTEST_APPLESS_MAIN(tst_AndroidAppless)
#include "tst_android_appless.moc"

