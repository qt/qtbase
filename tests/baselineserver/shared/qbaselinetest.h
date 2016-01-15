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

#ifndef BASELINETEST_H
#define BASELINETEST_H

#include <QTest>

namespace QBaselineTest {
void setAutoMode(bool mode);
void setSimFail(bool fail);
void handleCmdLineArgs(int *argcp, char ***argvp);
void addClientProperty(const QString& key, const QString& value);
bool connectToBaselineServer(QByteArray *msg = 0, const QString &testProject = QString(), const QString &testCase = QString());
bool checkImage(const QImage& img, const char *name, quint16 checksum, QByteArray *msg, bool *error, int manualdatatag = 0);
bool testImage(const QImage& img, QByteArray *msg, bool *error);
QTestData &newRow(const char *dataTag, quint16 checksum = 0);
bool disconnectFromBaselineServer();
}

#define QBASELINE_CHECK_SUM(image, name, checksum)\
do {\
    QByteArray _msg;\
    bool _err = false;\
    if (!QBaselineTest::checkImage((image), (name), (checksum), &_msg, &_err)) {\
        QFAIL(_msg.constData());\
    } else if (_err) {\
        QSKIP(_msg.constData());\
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
        QSKIP(_msg.constData());\
    }\
} while (0)

#endif // BASELINETEST_H
