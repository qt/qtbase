/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include <QLocale>
#include <QTest>

class tst_QLocale : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void toUpper_QLocale_1();
    void toUpper_QLocale_2();
    void toUpper_QString();
};

static QString data()
{
    return QStringLiteral("/qt-5/qtbase/tests/benchmarks/corelib/tools/qlocale");
}

#define LOOP(s) for (int i = 0; i < 5000; ++i) { s; }

void tst_QLocale::toUpper_QLocale_1()
{
    QString s = data();
    QBENCHMARK { LOOP(QLocale().toUpper(s)) }
}

void tst_QLocale::toUpper_QLocale_2()
{
    QString s = data();
    QLocale l;
    QBENCHMARK { LOOP(l.toUpper(s)) }
}

void tst_QLocale::toUpper_QString()
{
    QString s = data();
    QBENCHMARK { LOOP(s.toUpper()) }
}

QTEST_MAIN(tst_QLocale)

#include "main.moc"
