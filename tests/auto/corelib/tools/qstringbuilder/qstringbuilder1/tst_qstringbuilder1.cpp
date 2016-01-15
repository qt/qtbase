/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite module of the Qt Toolkit.
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


// SCENARIO 1
// this is the "no harm done" version. Only operator% is active,
// with NO_CAST * defined
#define P %
#undef QT_USE_QSTRINGBUILDER
#define QT_NO_CAST_FROM_ASCII
#define QT_NO_CAST_TO_ASCII


#include <QtTest/QtTest>

#define LITERAL "some literal"

void runScenario(); // Defined in stringbuilder.cpp #included below.

class tst_QStringBuilder1 : public QObject
{
    Q_OBJECT

private slots:
    void scenario() { runScenario(); }
};

#include "stringbuilder.cpp"
#include "tst_qstringbuilder1.moc"

QTEST_APPLESS_MAIN(tst_QStringBuilder1)
