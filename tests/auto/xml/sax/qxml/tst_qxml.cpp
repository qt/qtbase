/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qxml.h>

class tst_QXml : public QObject
{
Q_OBJECT

private slots:
    void getSetCheck();
    void interpretedAs0D() const;
#ifndef QT_NO_EXCEPTIONS
    void exception();
#endif
};

class MyXmlEntityResolver : public QXmlEntityResolver
{
public:
    MyXmlEntityResolver() : QXmlEntityResolver() {}
    QString errorString() const { return QString(); }
    bool resolveEntity(const QString &, const QString &, QXmlInputSource *&) { return false; }
};

class MyXmlContentHandler : public QXmlContentHandler
{
public:
    MyXmlContentHandler() : QXmlContentHandler() {}
    bool characters(const QString &) { return false; }
    bool endDocument() { return false; }
    bool endElement(const QString &, const QString &, const QString &) { return false; }
    bool endPrefixMapping(const QString &) { return false; }
    QString errorString() const { return QString(); }
    bool ignorableWhitespace(const QString &) { return false; }
    bool processingInstruction(const QString &, const QString &) { return false; }
    void setDocumentLocator(QXmlLocator *) { }
    bool skippedEntity(const QString &) { return false; }
    bool startDocument() { return false; }
    bool startElement(const QString &, const QString &, const QString &, const QXmlAttributes &) { return false; }
    bool startPrefixMapping(const QString &, const QString &) { return false; }
};

class MyXmlErrorHandler : public QXmlErrorHandler
{
public:
    MyXmlErrorHandler() : QXmlErrorHandler() {}
    QString errorString() const { return QString(); }
    bool error(const QXmlParseException &) { return false; }
    bool fatalError(const QXmlParseException &) { return false; }
    bool warning(const QXmlParseException &) { return false; }
};

class MyXmlLexicalHandler : public QXmlLexicalHandler
{
public:
    MyXmlLexicalHandler() : QXmlLexicalHandler() {}
    bool comment(const QString &) { return false; }
    bool endCDATA() { return false; }
    bool endDTD() { return false; }
    bool endEntity(const QString &) { return false; }
    QString errorString() const { return QString(); }
    bool startCDATA() { return false; }
    bool startDTD(const QString &, const QString &, const QString &) { return false; }
    bool startEntity(const QString &) { return false; }
};

class MyXmlDeclHandler : public QXmlDeclHandler
{
public:
    MyXmlDeclHandler() : QXmlDeclHandler() {}
    bool attributeDecl(const QString &, const QString &, const QString &, const QString &, const QString &) { return false; }
    QString errorString() const { return QString(); }
    bool externalEntityDecl(const QString &, const QString &, const QString &) { return false; }
    bool internalEntityDecl(const QString &, const QString &) { return false; }
};

// Testing get/set functions
void tst_QXml::getSetCheck()
{
    QXmlSimpleReader obj1;
    // QXmlEntityResolver* QXmlSimpleReader::entityResolver()
    // void QXmlSimpleReader::setEntityResolver(QXmlEntityResolver*)
    MyXmlEntityResolver *var1 = new MyXmlEntityResolver;
    obj1.setEntityResolver(var1);
    QCOMPARE(static_cast<QXmlEntityResolver *>(var1), obj1.entityResolver());
    obj1.setEntityResolver((QXmlEntityResolver *)0);
    QCOMPARE((QXmlEntityResolver *)0, obj1.entityResolver());
    delete var1;

    // QXmlContentHandler* QXmlSimpleReader::contentHandler()
    // void QXmlSimpleReader::setContentHandler(QXmlContentHandler*)
    MyXmlContentHandler *var2 = new MyXmlContentHandler;
    obj1.setContentHandler(var2);
    QCOMPARE(static_cast<QXmlContentHandler *>(var2), obj1.contentHandler());
    obj1.setContentHandler((QXmlContentHandler *)0);
    QCOMPARE((QXmlContentHandler *)0, obj1.contentHandler());
    delete var2;

    // QXmlErrorHandler* QXmlSimpleReader::errorHandler()
    // void QXmlSimpleReader::setErrorHandler(QXmlErrorHandler*)
    MyXmlErrorHandler *var3 = new MyXmlErrorHandler;
    obj1.setErrorHandler(var3);
    QCOMPARE(static_cast<QXmlErrorHandler *>(var3), obj1.errorHandler());
    obj1.setErrorHandler((QXmlErrorHandler *)0);
    QCOMPARE((QXmlErrorHandler *)0, obj1.errorHandler());
    delete var3;

    // QXmlLexicalHandler* QXmlSimpleReader::lexicalHandler()
    // void QXmlSimpleReader::setLexicalHandler(QXmlLexicalHandler*)
    MyXmlLexicalHandler *var4 = new MyXmlLexicalHandler;
    obj1.setLexicalHandler(var4);
    QCOMPARE(static_cast<QXmlLexicalHandler *>(var4), obj1.lexicalHandler());
    obj1.setLexicalHandler((QXmlLexicalHandler *)0);
    QCOMPARE((QXmlLexicalHandler *)0, obj1.lexicalHandler());
    delete var4;

    // QXmlDeclHandler* QXmlSimpleReader::declHandler()
    // void QXmlSimpleReader::setDeclHandler(QXmlDeclHandler*)
    MyXmlDeclHandler *var5 = new MyXmlDeclHandler;
    obj1.setDeclHandler(var5);
    QCOMPARE(static_cast<QXmlDeclHandler *>(var5), obj1.declHandler());
    obj1.setDeclHandler((QXmlDeclHandler *)0);
    QCOMPARE((QXmlDeclHandler *)0, obj1.declHandler());
    delete var5;
}

void tst_QXml::interpretedAs0D() const
{
    /* See task 172632. */

    class MyHandler : public QXmlDefaultHandler
    {
    public:
        virtual bool startElement(const QString &namespaceURI,
                                  const QString &localName,
                                  const QString &qName,
                                  const QXmlAttributes &atts)
        {
            Q_UNUSED(namespaceURI);
            Q_UNUSED(localName);
            Q_UNUSED(qName);
            attrName = atts.qName(0);
            attrCount = atts.count();
            return true;
        }

        QString attrName;
        int     attrCount;
    };

    const QString document(QLatin1String("<element ") +
                           QChar(0x010D) +
                           QLatin1String("reated-by=\"an attr value\"/>"));

    QString testFile = QFINDTESTDATA("0x010D.xml");
    if (testFile.isEmpty())
        QFAIL("Cannot find test file 0x010D.xml!");
    QFile f(testFile);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QXmlInputSource data(&f);

    QXmlSimpleReader reader;

    MyHandler myHandler;
    reader.setContentHandler(&myHandler);
    reader.setErrorHandler(&myHandler);

    QVERIFY(reader.parse(&data));

    QCOMPARE(myHandler.attrCount, 1);
    QCOMPARE(myHandler.attrName, QChar(0x010D) + QString::fromLatin1("reated-by"));
}

#ifndef QT_NO_EXCEPTIONS
void tst_QXml::exception()
{
    QString message = QString::fromLatin1("message");
    int column = 3;
    int line = 2;
    QString publicId = QString::fromLatin1("publicId");
    QString systemId = QString::fromLatin1("systemId");

    try {
        QXmlParseException e(message, column, line, publicId, systemId);
        throw e;
    }
    catch (QXmlParseException e) {
        QCOMPARE(e.message(), message);
        QCOMPARE(e.columnNumber(), column);
        QCOMPARE(e.lineNumber(), line);
        QCOMPARE(e.publicId(), publicId);
        QCOMPARE(e.systemId(), systemId);
    }
}
#endif

QTEST_MAIN(tst_QXml)
#include "tst_qxml.moc"
