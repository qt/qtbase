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


#include <QtTest/QtTest>

#include <qbytearraymatcher.h>

// COM interface
#if defined(Q_OS_WIN) && defined(interface)
#    undef interface
#endif

class tst_QByteArrayMatcher : public QObject
{
    Q_OBJECT

private slots:
    void interface();
    void indexIn();
};

static QByteArrayMatcher matcher1;

void tst_QByteArrayMatcher::interface()
{
    const char needle[] = "abc123";
    QByteArray haystack(500, 'a');
    haystack.insert(6, "123");
    haystack.insert(31, "abc");
    haystack.insert(42, "abc123");
    haystack.insert(84, "abc123");

    matcher1 = QByteArrayMatcher(QByteArray(needle));
    QByteArrayMatcher matcher2;
    matcher2.setPattern(QByteArray(needle));

    QByteArrayMatcher matcher3 = QByteArrayMatcher(QByteArray(needle));
    QByteArrayMatcher matcher4(needle, sizeof(needle) - 1);
    QByteArrayMatcher matcher5(matcher2);
    QByteArrayMatcher matcher6;
    matcher6 = matcher3;

    QCOMPARE(matcher1.indexIn(haystack), 42);
    QCOMPARE(matcher2.indexIn(haystack), 42);
    QCOMPARE(matcher3.indexIn(haystack), 42);
    QCOMPARE(matcher4.indexIn(haystack), 42);
    QCOMPARE(matcher5.indexIn(haystack), 42);
    QCOMPARE(matcher6.indexIn(haystack), 42);

    QCOMPARE(matcher1.indexIn(haystack.constData(), haystack.length()), 42);

    QCOMPARE(matcher1.indexIn(haystack, 43), 84);
    QCOMPARE(matcher1.indexIn(haystack.constData(), haystack.length(), 43), 84);
    QCOMPARE(matcher1.indexIn(haystack, 85), -1);
    QCOMPARE(matcher1.indexIn(haystack.constData(), haystack.length(), 85), -1);

    QByteArrayMatcher matcher7(QByteArray("123"));
    QCOMPARE(matcher7.indexIn(haystack), 6);

    matcher7 = QByteArrayMatcher(QByteArray("abc"));
    QCOMPARE(matcher7.indexIn(haystack), 31);

    matcher7.setPattern(matcher4.pattern());
    QCOMPARE(matcher7.indexIn(haystack), 42);
}


static QByteArrayMatcher matcher;

void tst_QByteArrayMatcher::indexIn()
{
    const char p_data[] = { 0x0, 0x0, 0x1 };
    QByteArray pattern(p_data, sizeof(p_data));

    QByteArray haystack(8, '\0');
    haystack[7] = 0x1;

    matcher = QByteArrayMatcher(pattern);
    QCOMPARE(matcher.indexIn(haystack, 0), 5);
    QCOMPARE(matcher.indexIn(haystack, 1), 5);
    QCOMPARE(matcher.indexIn(haystack, 2), 5);

    matcher.setPattern(pattern);
    QCOMPARE(matcher.indexIn(haystack, 0), 5);
    QCOMPARE(matcher.indexIn(haystack, 1), 5);
    QCOMPARE(matcher.indexIn(haystack, 2), 5);
}

QTEST_APPLESS_MAIN(tst_QByteArrayMatcher)
#include "tst_qbytearraymatcher.moc"
