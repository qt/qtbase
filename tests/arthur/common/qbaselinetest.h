/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef BASELINETEST_H
#define BASELINETEST_H

#include <QTest>

namespace QBaselineTest {
bool checkImage(const QImage& img, const char *name, quint16 checksum, QByteArray *msg, bool *error);
bool testImage(const QImage& img, QByteArray *msg, bool *error);
QTestData &newRow(const char *dataTag, quint16 checksum = 0);
}

#define QBASELINE_CHECK_SUM(image, name, checksum)\
do {\
    QByteArray _msg;\
    bool _err = false;\
    if (!QBaselineTest::checkImage((image), (name), (checksum), &_msg, &_err)) {\
        QFAIL(_msg.constData());\
    } else if (_err) {\
        QSKIP(_msg.constData(), SkipSingle);\
    }\
} while (0)

#define QBASELINE_CHECK(image, name) QBASELINE_CHECK_SUM(image, name, 0)

#define QBASELINE_TEST(image)\
do {\
    QByteArray _msg;\
    bool _err = false;\
    if (!QBaselineTest::testImage((image), &_msg, &_err)) {\
        QFAIL(_msg.constData());\
    } else if (_err) {\
        QSKIP(_msg.constData(), SkipSingle);\
    }\
} while (0)

#endif // BASELINETEST_H
