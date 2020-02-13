/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtCore/QCoreApplication>
#include <QtTest/QtTest>

class tst_BenchlibCounting : public QObject
{
    Q_OBJECT

private slots:
    void passingBenchmark();
    void skippingBenchmark();
    void failingBenchmark();
};

void tst_BenchlibCounting::passingBenchmark()
{
    QBENCHMARK {
    }
}

void tst_BenchlibCounting::skippingBenchmark()
{
    QBENCHMARK {
        QSKIP("This is a skipping benchmark");
    }
}

void tst_BenchlibCounting::failingBenchmark()
{
    QBENCHMARK {
        QFAIL("This is a failing benchmark");
    };
}

int main(int argc, char *argv[])
{
    std::vector<const char*> args(argv, argv + argc);
    args.push_back("-eventcounter");
    argc = args.size();
    argv = const_cast<char**>(&args[0]);

    QTEST_MAIN_IMPL(tst_BenchlibCounting)
}

#include "tst_benchlibcounting.moc"
