// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QtGui/qguiapplication.h>
#include <QtGui/qshortcut.h>
#include <QtGui/qwindow.h>

class tst_QShortcut : public QObject
{
    Q_OBJECT

private slots:
    void trigger();
};

void tst_QShortcut::trigger()
{
    QWindow w;
    new QShortcut(Qt::CTRL | Qt::Key_Q, &w, SLOT(close()));
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QTRY_VERIFY(QGuiApplication::applicationState() == Qt::ApplicationActive);
    QTest::sendKeyEvent(QTest::Click, &w, Qt::Key_Q, 'q', Qt::ControlModifier);
    QTRY_VERIFY(!w.isVisible());
}

QTEST_MAIN(tst_QShortcut)
#include "tst_qshortcut.moc"
