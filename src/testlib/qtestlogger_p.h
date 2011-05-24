/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtTest module of the Qt Toolkit.
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

#ifndef QTESTLOGGER_P_H
#define QTESTLOGGER_P_H

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

#include <QtTest/private/qabstracttestlogger_p.h>

QT_BEGIN_NAMESPACE

class QTestBasicStreamer;
class QTestElement;
class QTestFileLogger;

class QTestLogger : public QAbstractTestLogger
{
    public:
        QTestLogger(int fm = 0);
        ~QTestLogger();

        enum TestLoggerFormat
        {
            TLF_XML = 0,
            TLF_LightXml = 1,
            TLF_XunitXml = 2
        };

        void startLogging();
        void stopLogging();

        void enterTestFunction(const char *function);
        void leaveTestFunction();

        void addIncident(IncidentTypes type, const char *description,
                     const char *file = 0, int line = 0);
        void addBenchmarkResult(const QBenchmarkResult &result);
        void addTag(QTestElement* element);

        void addMessage(MessageTypes type, const char *message,
                    const char *file = 0, int line = 0);

        void setLogFormat(TestLoggerFormat fm);
        TestLoggerFormat logFormat();

        int passCount() const;
        int failureCount() const;
        int errorCount() const;
        int warningCount() const;
        int skipCount() const;
        int systemCount() const;
        int qdebugCount() const;
        int qwarnCount() const;
        int qfatalCount() const;
        int infoCount() const;
        void registerRandomSeed(unsigned int seed);
        unsigned int randomSeed() const;
        bool hasRandomSeed() const;

    private:
        QTestElement *listOfTestcases;
        QTestElement *currentLogElement;
        QTestElement *errorLogElement;
        QTestBasicStreamer *logFormatter;
        TestLoggerFormat format;
        QTestFileLogger *filelogger;

        int testCounter;
        int passCounter;
        int failureCounter;
        int errorCounter;
        int warningCounter;
        int skipCounter;
        int systemCounter;
        int qdebugCounter;
        int qwarnCounter;
        int qfatalCounter;
        int infoCounter;
        unsigned int randomSeed_;
        bool hasRandomSeed_;
};

QT_END_NAMESPACE

#endif // QTESTLOGGER_P_H
