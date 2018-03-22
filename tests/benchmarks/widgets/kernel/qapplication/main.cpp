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
#include <QApplication>

#include <qtest.h>


class tst_qapplication : public QObject
{
    Q_OBJECT
private slots:
    void ctor();
};

/*
    Test the performance of the QApplication constructor.

    Note: results from the second start on can be misleading,
    since all global statics are already initialized.
*/
void tst_qapplication::ctor()
{
    // simulate reasonable argc, argv
    int argc = 1;
    char *argv[] = { const_cast<char*>("tst_qapplication") };
    QBENCHMARK {
        QApplication app(argc, argv);
    }
}

QTEST_APPLESS_MAIN(tst_qapplication)

#include "main.moc"
