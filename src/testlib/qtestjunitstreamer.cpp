// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

    if (element->elementType() == QTest::LET_Text) {
        QTest::qt_asprintf(formatted, "%s<![CDATA[", indent);
        return;
    }

    QTest::qt_asprintf(formatted, "%s<%s", indent, element->elementName());
}

void QTestJUnitStreamer::formatEnd(const QTestElement *element, QTestCharBuffer *formatted) const
{
    if (!element || !formatted )
        return;

    if (element->childElements().empty()) {
        formatted->data()[0] = '\0';
        return;
    }

    char indent[20];
    indentForElement(element, indent, sizeof(indent));

    QTest::qt_asprintf(formatted, "%s</%s>\n", indent, element->elementName());
}

bool QTestJUnitStreamer::formatAttributes(const QTestElement* element,
                                          const QTestElementAttribute *attribute,
                                          QTestCharBuffer *formatted) const
{
    if (!attribute || !formatted )
        return false;

    QTest::AttributeIndex attrindex = attribute->index();

    if (element && element->elementType() == QTest::LET_Text) {
        QTEST_ASSERT(attrindex == QTest::AI_Value);
        return QXmlTestLogger::xmlCdata(formatted, attribute->value());
    }

    QTestCharBuffer quotedValue;
    if (QXmlTestLogger::xmlQuote(&quotedValue, attribute->value())) {
        return QTest::qt_asprintf(formatted, " %s=\"%s\"",
                                  attribute->name(), quotedValue.constData()) != 0;
    }
    return false;
}

void QTestJUnitStreamer::formatAfterAttributes(const QTestElement *element, QTestCharBuffer *formatted) const
{
    if (!element || !formatted )
        return;

    if (element->elementType() == QTest::LET_Text) {
        QTest::qt_asprintf(formatted, "]]>\n");
        return;
    }

    if (element->childElements().empty())
        QTest::qt_asprintf(formatted, "/>\n");
    else
        QTest::qt_asprintf(formatted, ">\n");
}

void QTestJUnitStreamer::output(QTestElement *element) const
{
    QTEST_ASSERT(element);

    if (!element->parentElement())
        outputString("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");

    QTestCharBuffer buf;

    formatStart(element, &buf);
    outputString(buf.data());

    outputElementAttributes(element, element->attributes());

    formatAfterAttributes(element, &buf);
    outputString(buf.data());

    if (!element->childElements().empty())
        outputElements(element->childElements());

    formatEnd(element, &buf);
    outputString(buf.data());
}

void QTestJUnitStreamer::outputElements(const std::vector<QTestElement*> &elements) const
{
    for (auto *element : elements)
        output(element);
}

void QTestJUnitStreamer::outputElementAttributes(const QTestElement* element, const std::vector<QTestElementAttribute*> &attributes) const
{
    QTestCharBuffer buf;

    for (auto *attribute : attributes) {
        if (formatAttributes(element, attribute, &buf))
            outputString(buf.data());
    }
}

void QTestJUnitStreamer::outputString(const char *msg) const
{
    testLogger->outputString(msg);
}

QT_END_NAMESPACE
