// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>
#include <qdesktopservices.h>
#include <qregularexpression.h>

using namespace Qt::StringLiterals;

class tst_qdesktopservices : public QObject
{
    Q_OBJECT

private slots:
    void openUrl();
    void handlers();
};

void tst_qdesktopservices::openUrl()
{
    // At the bare minimum check that they return false for invalid url's
    QCOMPARE(QDesktopServices::openUrl(QUrl()), false);
#if defined(Q_OS_WIN)
    // this test is only valid on windows on other systems it might mean open a new document in the application handling .file
    const QRegularExpression messagePattern("ShellExecute 'file://invalid\\.file' failed \\(error \\d+\\)\\.");
    QVERIFY(messagePattern.isValid());
    QTest::ignoreMessage(QtWarningMsg, messagePattern);
    QCOMPARE(QDesktopServices::openUrl(QUrl("file://invalid.file")), false);
#endif
}

class MyUrlHandler : public QObject
{
    Q_OBJECT
public:
    QUrl lastHandledUrl;

public slots:
    inline void handle(const QUrl &url) {
        lastHandledUrl = url;
    }
};

#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
# define CAN_IMPLICITLY_UNSET
#endif

void tst_qdesktopservices::handlers()
{
    MyUrlHandler fooHandler;
    MyUrlHandler barHandler;

    QDesktopServices::setUrlHandler(QString("foo"), &fooHandler, "handle");
    QDesktopServices::setUrlHandler(QString("bar"), &barHandler, "handle");
#ifndef CAN_IMPLICITLY_UNSET
    const auto unsetHandlers = qScopeGuard([] {
        QDesktopServices::unsetUrlHandler(u"bar"_s);
        QDesktopServices::unsetUrlHandler(u"foo"_s);
    });
#endif

    QUrl fooUrl("foo://blub/meh");
    QUrl barUrl("bar://hmm/hmmmm");

    QVERIFY(QDesktopServices::openUrl(fooUrl));
    QVERIFY(QDesktopServices::openUrl(barUrl));

    QCOMPARE(fooHandler.lastHandledUrl.toString(), fooUrl.toString());
    QCOMPARE(barHandler.lastHandledUrl.toString(), barUrl.toString());

#ifdef CAN_IMPLICITLY_UNSET
    for (int i = 0; i < 2; ++i)
        QTest::ignoreMessage(QtWarningMsg,
                             "Please call QDesktopServices::unsetUrlHandler() before destroying a "
                             "registered URL handler object.\n"
                             "Support for destroying a registered URL handler object is deprecated, "
                             "and will be removed in Qt 6.6.");
#endif
}

QTEST_MAIN(tst_qdesktopservices)

#include "tst_qdesktopservices.moc"
