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

#include "qtestbasicstreamer.h"
#include "qtestlogger_p.h"
#include "qtestelement.h"
#include "qtestelementattribute.h"
#include "QtTest/private/qtestlog_p.h"
#include "qtestassert.h"

#include <stdio.h>
#include <stdlib.h>

#ifndef Q_OS_WIN
#include <unistd.h>
#endif

QT_BEGIN_NAMESPACE

namespace QTest
{
    static FILE *stream = 0;
}

QTestBasicStreamer::QTestBasicStreamer()
    :testLogger(0)
{
}

QTestBasicStreamer::~QTestBasicStreamer()
{}

void QTestBasicStreamer::formatStart(const QTestElement *element, QTestCharBuffer *formatted) const
{
    if(!element || !formatted )
        return;
    formatted->data()[0] = '\0';
}

void QTestBasicStreamer::formatEnd(const QTestElement *element, QTestCharBuffer *formatted) const
{
    if(!element || !formatted )
        return;
    formatted->data()[0] = '\0';
}

void QTestBasicStreamer::formatBeforeAttributes(const QTestElement *element, QTestCharBuffer *formatted) const
{
    if(!element || !formatted )
        return;
    formatted->data()[0] = '\0';
}

void QTestBasicStreamer::formatAfterAttributes(const QTestElement *element, QTestCharBuffer *formatted) const
{
    if(!element || !formatted )
        return;
    formatted->data()[0] = '\0';
}

void QTestBasicStreamer::formatAttributes(const QTestElement *, const QTestElementAttribute *attribute, QTestCharBuffer *formatted) const
{
    if(!attribute || !formatted )
        return;
    formatted->data()[0] = '\0';
}

void QTestBasicStreamer::output(QTestElement *element) const
{
    if(!element)
        return;

    outputElements(element);
}

void QTestBasicStreamer::outputElements(QTestElement *element, bool) const
{
    QTestCharBuffer buf;
    bool hasChildren;
    /*
        Elements are in reverse order of occurrence, so start from the end and work
        our way backwards.
    */
    while (element && element->nextElement()) {
        element = element->nextElement();
    }
    while (element) {
        hasChildren = element->childElements();

        formatStart(element, &buf);
        outputString(buf.data());

        formatBeforeAttributes(element, &buf);
        outputString(buf.data());

        outputElementAttributes(element, element->attributes());

        formatAfterAttributes(element, &buf);
        outputString(buf.data());

        if(hasChildren)
            outputElements(element->childElements(), true);

        formatEnd(element, &buf);
        outputString(buf.data());

        element = element->previousElement();
    }
}

void QTestBasicStreamer::outputElementAttributes(const QTestElement* element, QTestElementAttribute *attribute) const
{
    QTestCharBuffer buf;
    while(attribute){
        formatAttributes(element, attribute, &buf);
        outputString(buf.data());
        attribute = attribute->nextElement();
    }
}

void QTestBasicStreamer::outputString(const char *msg) const
{
    QTEST_ASSERT(QTest::stream);

    ::fputs(msg, QTest::stream);
    ::fflush(QTest::stream);
}

void QTestBasicStreamer::startStreaming()
{
    QTEST_ASSERT(!QTest::stream);

    const char *out = QTestLog::outputFileName();
    if (!out) {
        QTest::stream = stdout;
        return;
    }
    #if defined(_MSC_VER) && _MSC_VER >= 1400 && !defined(Q_OS_WINCE)
    if (::fopen_s(&QTest::stream, out, "wt")) {
        #else
        QTest::stream = ::fopen(out, "wt");
        if (!QTest::stream) {
            #endif
            printf("Unable to open file for logging: %s", out);
            ::exit(1);
        }
}

bool QTestBasicStreamer::isTtyOutput()
{
    QTEST_ASSERT(QTest::stream);

#if defined(Q_OS_WIN) || defined(Q_OS_INTEGRITY)
    return true;
#else
    static bool ttyoutput = isatty(fileno(QTest::stream));
    return ttyoutput;
#endif
}

void QTestBasicStreamer::stopStreaming()
{
    QTEST_ASSERT(QTest::stream);
    if (QTest::stream != stdout)
        fclose(QTest::stream);

    QTest::stream = 0;
}

void QTestBasicStreamer::setLogger(const QTestLogger *tstLogger)
{
    testLogger = tstLogger;
}

const QTestLogger *QTestBasicStreamer::logger() const
{
    return testLogger;
}

QT_END_NAMESPACE

