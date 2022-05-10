// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef BASELINETEST_H
#define BASELINETEST_H

#include <QTest>
#include <QString>

namespace QBaselineTest {
void setAutoMode(bool mode);
void setSimFail(bool fail);
void handleCmdLineArgs(int *argcp, char ***argvp);
void setProject(const QString &projectName); // Selects server config settings and top level dir
void setProjectImageKeys(const QStringList &keys); // Overrides the ItemPathKeys config setting
void addClientProperty(const QString& key, const QString& value);
bool connectToBaselineServer(QByteArray *msg = nullptr);
bool checkImage(const QImage& img, const char *name, quint16 checksum, QByteArray *msg, bool *error, int manualdatatag = 0);
bool testImage(const QImage& img, QByteArray *msg, bool *error);
QTestData &newRow(const char *dataTag, quint16 checksum = 0);
bool disconnectFromBaselineServer();
bool shouldAbortIfUnstable();
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

#define QBASELINE_CHECK_SUM_DEFERRED(image, name, checksum)\
do {\
    QByteArray _msg;\
    bool _err = false;\
    if (!QBaselineTest::checkImage((image), (name), (checksum), &_msg, &_err)) {\
        QTest::qFail(_msg.constData(), __FILE__, __LINE__);\
    } else if (_err) {\
        QSKIP(_msg.constData());\
    }\
} while (0)

#define QBASELINE_CHECK(image, name) QBASELINE_CHECK_SUM(image, name, 0)

#define QBASELINE_CHECK_DEFERRED(image, name) QBASELINE_CHECK_SUM_DEFERRED(image, name, 0)

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
