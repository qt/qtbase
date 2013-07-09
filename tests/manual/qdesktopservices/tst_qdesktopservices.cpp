/****************************************************************************
**
** Copyright (C) 2013 Intel Corporation.
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
    QDesktopServices::openUrl(data);
}

QTEST_MAIN(tst_QDesktopServices)

#include "tst_qdesktopservices.moc"
