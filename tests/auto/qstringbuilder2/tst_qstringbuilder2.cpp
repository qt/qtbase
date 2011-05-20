/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


// SCENARIO 2
// this is the "full" version. Operator+ is replaced by a QStringBuilder
// based version
// with NO_CAST * defined
#define P +
#define QT_USE_QSTRINGBUILDER
#define QT_NO_CAST_FROM_ASCII
#define QT_NO_CAST_TO_ASCII


#include <QtTest/QtTest>

//TESTED_CLASS=QStringBuilder
//TESTED_FILES=qstringbuilder.cpp

#define LITERAL "some literal"

void runScenario(); // Defined in stringbuilder.cpp #included below.

class tst_QStringBuilder2 : public QObject
{
    Q_OBJECT

private slots:
    void scenario() { runScenario(); }
};

#include "../qstringbuilder1/stringbuilder.cpp"
#include "tst_qstringbuilder2.moc"

QTEST_APPLESS_MAIN(tst_QStringBuilder2)
