/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtTest module of the Qt Toolkit.
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

#ifndef QBENCHMARKVALGRIND_P_H
#define QBENCHMARKVALGRIND_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtTest/private/qbenchmarkmeasurement_p.h"
#include "QtTest/private/qbenchmarkmetric_p.h"
#include <QtCore/qmap.h>
#include <QtCore/qstring.h>

class QStringList;

QT_BEGIN_NAMESPACE

class QBenchmarkValgrindUtils
{
public:
    static bool haveValgrind();
    static bool rerunThroughCallgrind(const QStringList &origAppArgs, int &exitCode);
    static bool runCallgrindSubProcess(const QStringList &origAppArgs, int &exitCode);
    static qint64 extractResult(const QString &fileName);
    static QString getNewestFileName();
    static qint64 extractLastResult();
    static void cleanup();
    static QString outFileBase(qint64 pid = -1);
};

class QBenchmarkCallgrindMeasurer : public QBenchmarkMeasurerBase
{
public:
    void start();
    qint64 checkpoint();
    qint64 stop();
    bool isMeasurementAccepted(qint64 measurement);
    int adjustIterationCount(int);
    int adjustMedianCount(int);
    bool needsWarmupIteration();
    QTest::QBenchmarkMetric metricType();
};

QT_END_NAMESPACE

#endif // QBENCHMARKVALGRIND_H
