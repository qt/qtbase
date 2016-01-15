/****************************************************************************
**
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <QDesktopServices>

class tst_QDesktopServices : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void openUrl();
    void openUrl_data();
};

void tst_QDesktopServices::openUrl_data()
{
    QTest::addColumn<QUrl>("data");
    QTest::addColumn<QString>("message");

    QUrl localFile = QUrl::fromLocalFile(QFINDTESTDATA("test.txt"));

    QTest::newRow("text-file")
            << localFile
            << "This should open test.txt in a text editor";

    localFile.setQuery("x=y");
    QTest::newRow("text-file-with-query")
            << localFile
            << "This should open test.txt in a text editor. Queries do not usually show up.";

    localFile.setQuery(QString());
    localFile.setFragment("top");
    QTest::newRow("text-file-with-fragment")
            << localFile
            << "This should open test.txt in a text editor. Fragments do not usually show up.";

    QTest::newRow("browser-plain")
            << QUrl("http://qt-project.org")
            << "This should open http://qt-project.org in the default web browser";

    QTest::newRow("search-url")
            << QUrl("http://google.com/search?q=Qt+Project")
            << "This should search \"Qt Project\" on Google";

    QTest::newRow("search-url-with-space")
            << QUrl("http://google.com/search?q=Qt Project")
            << "This should search \"Qt Project\" on Google";

    QTest::newRow("search-url-with-quotes")
            << QUrl("http://google.com/search?q=\"Qt+Project\"")
            << "This should search '\"Qt Project\"' on Google (including the quotes)";

    QTest::newRow("search-url-with-hashtag")
            << QUrl("http://google.com/search?q=%23qtproject")
            << "This should search \"#qtproject\" on Google. The # should appear in the Google search field";

    QTest::newRow("search-url-with-fragment")
            << QUrl("http://google.com/search?q=Qt+Project#top")
            << "This should search \"Qt Project\" on Google. There should be no # in the Google search field";

    // see QTBUG-32311
    QTest::newRow("search-url-with-slashes")
            << QUrl("http://google.com/search?q=/profile/5")
            << "This should search \"/profile/5\" on Google.";

    // see QTBUG-31945
    QTest::newRow("two-fragments")
            << QUrl("http://developer.apple.com/library/ios/#documentation/uikit/reference/UIViewController_Class/Reference/Reference.html#//apple_ref/doc/uid/TP40006926-CH3-SW81")
            << "This should open \"Implementing a Container View Controller\" in the UIViewController docs";

    QTest::newRow("mail")
            << QUrl("mailto:development@qt-project.org")
            << "This should open an email composer with the destination set to development@qt-project.org";

    QTest::newRow("mail-subject")
            << QUrl("mailto:development@qt-project.org?subject=[Development]%20Test%20Mail")
            << "This should open an email composer and tries to set the subject";
}

void tst_QDesktopServices::openUrl()
{
    QFETCH(QUrl, data);
    QFETCH(QString, message);
    qWarning("\n\nOpening \"%s\": %s", qPrintable(data.toString()), qPrintable(message));
    QVERIFY(QDesktopServices::openUrl(data));
}

QTEST_MAIN(tst_QDesktopServices)

#include "tst_qdesktopservices.moc"
