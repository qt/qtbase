/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/private/qtestjunitstreamer_p.h>
#include <QtTest/private/qjunittestlogger_p.h>
#include <QtTest/private/qtestelement_p.h>
#include <QtTest/private/qtestelementattribute_p.h>
#include <QtTest/qtestassert.h>
#include <QtTest/private/qtestlog_p.h>
#include <QtTest/private/qtestresult_p.h>
#include <QtTest/private/qxmltestlogger_p.h>

QT_BEGIN_NAMESPACE

QTestJUnitStreamer::QTestJUnitStreamer(QJUnitTestLogger *logger)
    : testLogger(logger)
{
    QTEST_ASSERT(testLogger);
}

QTestJUnitStreamer::~QTestJUnitStreamer() = default;

void QTestJUnitStreamer::indentForElement(const QTestElement* element, char* buf, int size)
{
    if (size == 0) return;

    buf[0] = 0;

    if (!element) return;

    char* endbuf = buf + size;
    element = element->parentElement();
    while (element && buf+2 < endbuf) {
        *(buf++) = ' ';
        *(buf++) = ' ';
        *buf = 0;
        element = element->parentElement();
    }
}

void QTestJUnitStreamer::formatStart(const QTestElement *element, QTestCharBuffer *formatted) const
{
    if (!element || !formatted )
        return;

    char indent[20];
    indentForElement(element, indent, sizeof(indent));

    // Errors are written as CDATA within system-err, comments elsewhere
    if (element->elementType() == QTest::LET_Error) {
        if (element->parentElement()->elementType() == QTest::LET_SystemError) {
            QTest::qt_asprintf(formatted, "<![CDATA[");
        } else {
            QTest::qt_asprintf(formatted, "%s<!--", indent);
        }
        return;
    }

    QTest::qt_asprintf(formatted, "%s<%s", indent, element->elementName());
}

void QTestJUnitStreamer::formatEnd(const QTestElement *element, QTestCharBuffer *formatted) const
{
    if (!element || !formatted )
        return;

    if (!element->childElements()) {
        formatted->data()[0] = '\0';
        return;
    }

    char indent[20];
    indentForElement(element, indent, sizeof(indent));

    QTest::qt_asprintf(formatted, "%s</%s>\n", indent, element->elementName());
}

void QTestJUnitStreamer::formatAttributes(const QTestElement* element, const QTestElementAttribute *attribute, QTestCharBuffer *formatted) const
{
    if (!attribute || !formatted )
        return;

    QTest::AttributeIndex attrindex = attribute->index();

    // For errors within system-err, we only want to output `message'
    if (element && element->elementType() == QTest::LET_Error
        && element->parentElement()->elementType() == QTest::LET_SystemError) {

        if (attrindex != QTest::AI_Description) return;

        QXmlTestLogger::xmlCdata(formatted, attribute->value());
        return;
    }

    char const* key = nullptr;
    if (attrindex == QTest::AI_Description)
        key = "message";
    else if (attrindex != QTest::AI_File && attrindex != QTest::AI_Line)
        key = attribute->name();

    if (key) {
        QTestCharBuffer quotedValue;
        QXmlTestLogger::xmlQuote(&quotedValue, attribute->value());
        QTest::qt_asprintf(formatted, " %s=\"%s\"", key, quotedValue.constData());
    } else {
        formatted->data()[0] = '\0';
    }
}

void QTestJUnitStreamer::formatAfterAttributes(const QTestElement *element, QTestCharBuffer *formatted) const
{
    if (!element || !formatted )
        return;

    // Errors are written as CDATA within system-err, comments elsewhere
    if (element->elementType() == QTest::LET_Error) {
        if (element->parentElement()->elementType() == QTest::LET_SystemError) {
            QTest::qt_asprintf(formatted, "]]>\n");
        } else {
            QTest::qt_asprintf(formatted, " -->\n");
        }
        return;
    }

    if (!element->childElements())
        QTest::qt_asprintf(formatted, "/>\n");
    else
        QTest::qt_asprintf(formatted, ">\n");
}

void QTestJUnitStreamer::output(QTestElement *element) const
{
    QTEST_ASSERT(element);

    outputString("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
    outputElements(element);
}

void QTestJUnitStreamer::outputElements(QTestElement *element, bool) const
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

        if (element->elementType() != QTest::LET_Benchmark) {
            formatStart(element, &buf);
            outputString(buf.data());

            outputElementAttributes(element, element->attributes());

            formatAfterAttributes(element, &buf);
            outputString(buf.data());

            if (hasChildren)
                outputElements(element->childElements(), true);

            formatEnd(element, &buf);
            outputString(buf.data());
        }
        element = element->previousElement();
    }
}

void QTestJUnitStreamer::outputElementAttributes(const QTestElement* element, QTestElementAttribute *attribute) const
{
    QTestCharBuffer buf;
    while (attribute) {
        formatAttributes(element, attribute, &buf);
        outputString(buf.data());
        attribute = attribute->nextElement();
    }
}

void QTestJUnitStreamer::outputString(const char *msg) const
{
    testLogger->outputString(msg);
}

QT_END_NAMESPACE
