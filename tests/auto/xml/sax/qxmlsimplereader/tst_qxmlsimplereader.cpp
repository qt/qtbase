/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QDomDocument>
#include <qthread.h>
#include <qtcpserver.h>
#include <qtcpsocket.h>
#include <QtTest/QtTest>
#include <qfile.h>
#include <qstring.h>
#include <qdir.h>
#include <qbuffer.h>
#include "parser/parser.h"

static const char *const inputString = "<!DOCTYPE inferno [<!ELEMENT inferno (circle+)><!ELEMENT circle (#PCDATA)>]><inferno><circle /><circle /></inferno>";
static const char *const refString = "setDocumentLocator(locator={columnNumber=1, lineNumber=1})\nstartDocument()\nstartDTD(name=\"inferno\", publicId=\"\", systemId=\"\")\nendDTD()\nstartElement(namespaceURI=\"\", localName=\"inferno\", qName=\"inferno\", atts=[])\nstartElement(namespaceURI=\"\", localName=\"circle\", qName=\"circle\", atts=[])\nendElement(namespaceURI=\"\", localName=\"circle\", qName=\"circle\")\nstartElement(namespaceURI=\"\", localName=\"circle\", qName=\"circle\", atts=[])\nendElement(namespaceURI=\"\", localName=\"circle\", qName=\"circle\")\nendElement(namespaceURI=\"\", localName=\"inferno\", qName=\"inferno\")\nendDocument()\n";

#define TEST_PORT 1088

class XmlServer : public QThread
{
    Q_OBJECT
public:
    XmlServer(QObject *parent = 0) : QThread(parent), quit_soon(false), listening(false) {}

    bool quit_soon;
    bool listening;

protected:
    virtual void run();
};

#define CHUNK_SIZE 2048

void XmlServer::run()
{
    QTcpServer srv;

    listening = srv.listen(QHostAddress::Any, TEST_PORT);
    if (!listening) {
        qWarning() << "Failed to listen on" << TEST_PORT << srv.errorString();
        return;
    }

    for (;;) {
        srv.waitForNewConnection(100);

        if (QTcpSocket *sock = srv.nextPendingConnection()) {
            QByteArray fileName;
            for (;;) {
                char c;
                if (sock->getChar(&c)) {
                    if (c == '\n')
                        break;
                    fileName.append(c);
                } else {
                    if (!sock->waitForReadyRead(-1))
                        break;
                }
            }

            QFile file(QString::fromLocal8Bit(fileName));
            if (!file.open(QIODevice::ReadOnly)) {
                qWarning() << "XmlServer::run(): could not open" << fileName;
                sock->abort();
                delete sock;
                continue;
            }

            QByteArray data = file.readAll();
            for (int i = 0; i < data.size();) {
//                sock->putChar(data.at(i));
                int cnt = qMin(CHUNK_SIZE, data.size() - i);
                sock->write(data.constData() + i, cnt);
                i += cnt;
                sock->flush();
                QTest::qSleep(1);

                if (quit_soon) {
                    sock->abort();
                    break;
                }
            }

            sock->disconnectFromHost();
            delete sock;
        }

        if (quit_soon)
            break;
    }

    srv.close();
}

class tst_QXmlSimpleReader : public QObject
{
    Q_OBJECT

    public:
        tst_QXmlSimpleReader();
        ~tst_QXmlSimpleReader();

    private slots:
        void initTestCase();
        void testGoodXmlFile();
        void testGoodXmlFile_data();
        void testBadXmlFile();
        void testBadXmlFile_data();
        void testIncrementalParsing();
        void testIncrementalParsing_data();
        void setDataQString();
        void inputFromQIODevice();
        void inputFromString();
        void inputFromSocket_data();
        void inputFromSocket();

        void idsInParseException1();
        void idsInParseException2();
        void preserveCharacterReferences() const;
        void reportNamespace() const;
        void reportNamespace_data() const;
        void roundtripWithNamespaces() const;
        void dtdRecursionLimit();

    private:
        static QDomDocument fromByteArray(const QString &title, const QByteArray &ba, bool *ok);
        XmlServer *server;
        QString prefix;
};

tst_QXmlSimpleReader::tst_QXmlSimpleReader() : server(new XmlServer(this))
{
    server->start();
}

tst_QXmlSimpleReader::~tst_QXmlSimpleReader()
{
    server->quit_soon = true;
    server->wait();
}

class MyErrorHandler : public QXmlErrorHandler
{
public:
    QString publicId;
    QString systemId;

    virtual bool error(const QXmlParseException &)
    {
        return false;
    }

    virtual QString errorString() const
    {
        return QString();
    }

    virtual bool fatalError(const QXmlParseException &exception)
    {
        publicId = exception.publicId();
        systemId = exception.systemId();
        return true;
    }

    virtual bool warning(const QXmlParseException &)
    {
        return true;
    }

};

void tst_QXmlSimpleReader::initTestCase()
{
    prefix = QFileInfo(QFINDTESTDATA("xmldocs")).absolutePath();
    if (prefix.isEmpty())
        QFAIL("Cannot find xmldocs testdata!");
    QDir::setCurrent(prefix);
}

void tst_QXmlSimpleReader::idsInParseException1()
{
    MyErrorHandler handler;
    QXmlSimpleReader reader;

    reader.setErrorHandler(&handler);

    /* A non-wellformed XML document with PUBLIC and SYSTEM. */
    QByteArray input("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" "
                     "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">"
                     "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">"
                     "<head>"
                         "<a/><a/><a/>"
                     "<head/>");

    QBuffer buff(&input);
    QXmlInputSource source(&buff);

    /* Yes, parsing should be reported as a failure. */
    QVERIFY(!reader.parse(source));

    QCOMPARE(handler.publicId, QString::fromLatin1("-//W3C//DTD XHTML 1.0 Strict//EN"));
    QCOMPARE(handler.systemId, QString::fromLatin1("http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd"));
}

void tst_QXmlSimpleReader::idsInParseException2()
{
    MyErrorHandler handler;
    QXmlSimpleReader reader;

    reader.setErrorHandler(&handler);

    /* A non-wellformed XML document with only SYSTEM. */
    QByteArray input("<!DOCTYPE html SYSTEM \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">"
                      "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">"
                      "<head>"
                          "<a/><a/><a/>"
                      "<head/>");

    QBuffer buff(&input);
    QXmlInputSource source(&buff);

    /* Yes, parsing should be reported as a failure. */
    QVERIFY(!reader.parse(source));

    QCOMPARE(handler.publicId, QString());
    QCOMPARE(handler.systemId, QString::fromLatin1("http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd"));
}

static QStringList findXmlFiles(QString dir_name)
{
    QStringList result;

    QDir dir(dir_name);
    QFileInfoList file_list = dir.entryInfoList(QStringList("*.xml"), QDir::Files, QDir::Name);

    QFileInfoList::const_iterator it = file_list.begin();
    for (; it != file_list.end(); ++it) {
        const QFileInfo &file_info = *it;
        result.append(file_info.filePath());
    }

    return result;
}


void tst_QXmlSimpleReader::testGoodXmlFile_data()
{
    const char * const good_data_dirs[] = {
        "xmldocs/valid/sa",
        "xmldocs/valid/not-sa",
        "xmldocs/valid/ext-sa",
        0
    };
    const char * const *d = good_data_dirs;

    QStringList good_file_list;
    for (; *d != 0; ++d)
        good_file_list += findXmlFiles(*d);

    QTest::addColumn<QString>("file_name");
    QStringList::const_iterator it = good_file_list.begin();
    for (; it != good_file_list.end(); ++it)
        QTest::newRow((*it).toLatin1()) << *it;
}

void tst_QXmlSimpleReader::testGoodXmlFile()
{
    QFETCH(QString, file_name);
    QFile file(file_name);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QString content = file.readAll();
    file.close();
    QVERIFY(file.open(QIODevice::ReadOnly));
    Parser parser;

    QEXPECT_FAIL("xmldocs/valid/sa/089.xml", "a form feed character is not accepted in XML", Continue);
    QVERIFY(parser.parseFile(&file));

    QFile ref_file(file_name + ".ref");
    QVERIFY(ref_file.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream ref_stream(&ref_file);
    ref_stream.setCodec("UTF-8");
    QString ref_file_contents = ref_stream.readAll();

    QCOMPARE(parser.result(), ref_file_contents);
}

void tst_QXmlSimpleReader::testBadXmlFile_data()
{
    const char * const bad_data_dirs[] = {
        "xmldocs/not-wf/sa",
        0
    };
    const char * const *d = bad_data_dirs;

    QStringList bad_file_list;
    for (; *d != 0; ++d)
        bad_file_list += findXmlFiles(*d);

    QTest::addColumn<QString>("file_name");
    QStringList::const_iterator it = bad_file_list.begin();
    for (; it != bad_file_list.end(); ++it)
        QTest::newRow((*it).toLatin1()) << *it;
}

void tst_QXmlSimpleReader::testBadXmlFile()
{
    QFETCH(QString, file_name);
    QFile file(file_name);
    QVERIFY(file.open(QIODevice::ReadOnly));
    Parser parser;

    QEXPECT_FAIL("xmldocs/not-wf/sa/030.xml", "a form feed character is not accepted in XML", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/031.xml", "a form feed character is not accepted in a processing instruction", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/032.xml", "a form feed character is not accepted in a comment", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/033.xml", "overlong sequence - small latin letter d should be rejected", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/038.xml", "attribute x redefined; should be rejected", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/072.xml", "entity foo not defined", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/073.xml", "entity f not defined", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/074.xml", "entity e is not well-formed (</foo><foo>)", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/076.xml", "entity foo is not defined", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/077.xml", "entity bar is not defined within the definition of entity foo", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/078.xml", "entity foo not defined", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/085.xml", "Unfinished Public or System Id", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/086.xml", "Unfinished Public or System Id", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/087.xml", "Unfinished Public or System Id", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/101.xml", "Invalid XML encoding name (space before utf-8)", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/102.xml", "Invalid version specification (1.0 followed by space)", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/104.xml", "Premature end of data in tag foo", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/116.xml", "Invalid decimal value", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/117.xml", "No name", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/119.xml", "No name", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/122.xml", "; expected in declaration of element", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/132.xml", "; expected in declaration of element", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/142.xml", "Invalid value '0'", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/143.xml", "Invalid value '31'", Continue);

    QEXPECT_FAIL("xmldocs/not-wf/sa/144.xml", "noncharacter code 0xFFFF should be rejected", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/145.xml", "surrogate code point 0xD800 should be rejected", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/146.xml", "code point out-of-range 0x110000 (must be < 0x10FFFE)", Abort);
    QEXPECT_FAIL("xmldocs/not-wf/sa/160.xml", "Parameter references forbidden in internal subset", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/162.xml", "Parameter references forbidden in internal subset", Continue);

    QEXPECT_FAIL("xmldocs/not-wf/sa/168.xml", "Surrogate code point 0xEDA080 should be rejected", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/169.xml", "Surrogate code point 0xEDB080 should be rejected", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/170.xml", "Code point 0xF7808080 should be rejected", Continue);

    QEXPECT_FAIL("xmldocs/not-wf/sa/180.xml", "Entity e is not defined", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/181.xml", "Unregistered error message", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/182.xml", "Comment not terminated", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/185.xml", "Entity e not defined", Continue);
    QEXPECT_FAIL("xmldocs/not-wf/sa/186.xml", "Attributes constructs error", Continue);

    QVERIFY(!parser.parseFile(&file));

    QFile ref_file(file_name + ".ref");
    QVERIFY(ref_file.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream ref_stream(&ref_file);
    ref_stream.setCodec("UTF-8");
    QString ref_file_contents = ref_stream.readAll();

    QEXPECT_FAIL("xmldocs/not-wf/sa/145.xml", "Surrogate code point 0xD800 should be rejected", Continue);

    QCOMPARE(parser.result(), ref_file_contents);
}

void tst_QXmlSimpleReader::testIncrementalParsing_data()
{
    QTest::addColumn<QString>("file_name");
    QTest::addColumn<int>("chunkSize");

    const char * const good_data_dirs[] = {
        "xmldocs/valid/sa",
        "xmldocs/valid/not-sa",
        "xmldocs/valid/ext-sa",
        0
    };
    const char * const *d = good_data_dirs;

    QStringList good_file_list;
    for (; *d != 0; ++d)
        good_file_list += findXmlFiles(*d);

    for (int i=1; i<10; ++i) {
        QStringList::const_iterator it = good_file_list.begin();
        for (; it != good_file_list.end(); ++it) {
            if ( *it == "xmldocs/valid/sa/089.xml" )
                continue;// TODO: fails at the moment -- don't bother
            if ( i==1 && (
                        *it == "xmldocs/valid/sa/049.xml" ||
                        *it == "xmldocs/valid/sa/050.xml" ||
                        *it == "xmldocs/valid/sa/051.xml" ||
                        *it == "xmldocs/valid/sa/052.xml" ) ) {
                continue; // TODO: fails at the moment -- don't bother
            }
            QTest::newRow(QString("%1 %2").arg(*it).arg(i).toLatin1()) << *it << i;
        }
    }
}

void tst_QXmlSimpleReader::testIncrementalParsing()
{
    QFETCH(QString, file_name);
    QFETCH(int, chunkSize);

    QFile file(file_name);
    QVERIFY(file.open(QIODevice::ReadOnly));

    Parser parser;
    QXmlInputSource source;
    bool first = true;
    while (!file.atEnd()) {
        source.setData(file.read(chunkSize));
        if(first) {
            QVERIFY(parser.parse(&source, true));
            first = false;
        } else {
            QVERIFY(parser.parseContinue());
        }
    }
    // detect end of document
    QVERIFY(parser.parseContinue());
    // parsing should fail after the end of the document was reached
    QVERIFY(!parser.parseContinue());

    QFile ref_file(file_name + ".ref");
    QVERIFY(ref_file.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream ref_stream(&ref_file);
    ref_stream.setCodec("UTF-8");
    QString ref_file_contents = ref_stream.readAll();

    QCOMPARE(parser.result(), ref_file_contents);
}

void tst_QXmlSimpleReader::setDataQString()
{
    QString input = inputString;
    QString ref = refString;

    QXmlInputSource source;
    Parser parser;

    source.setData(input);
    QVERIFY(parser.parse(&source,false));

    QBuffer resultBuffer;
    resultBuffer.setData(parser.result().toLatin1());

    QBuffer refBuffer;
    refBuffer.setData(ref.toLatin1());

    resultBuffer.open(QIODevice::ReadOnly);
    refBuffer.open(QIODevice::ReadOnly);

    bool success = true;
    while (resultBuffer.canReadLine()) {
        if (!refBuffer.canReadLine()) {
            success = false; break ;
        }
        if (resultBuffer.readLine().simplified() != refBuffer.readLine().simplified()) {
            success = false; break ;
        }
    }
    QVERIFY(success);
}

void tst_QXmlSimpleReader::inputFromQIODevice()
{
    QBuffer inputBuffer;
    inputBuffer.setData(inputString);

    QXmlInputSource source(&inputBuffer);
    Parser parser;

    QVERIFY(parser.parse(&source,false));

    QBuffer resultBuffer;
    resultBuffer.setData(parser.result().toLatin1());

    QBuffer refBuffer;
    refBuffer.setData(refString);

    resultBuffer.open(QIODevice::ReadOnly);
    refBuffer.open(QIODevice::ReadOnly);

    bool success = true;
    while (resultBuffer.canReadLine()) {
        if (!refBuffer.canReadLine()) {
            success = false; break ;
        }
        if (resultBuffer.readLine().simplified() != refBuffer.readLine().simplified()) {
            success = false; break ;
        }
    }
    QVERIFY(success);
}

void tst_QXmlSimpleReader::inputFromString()
{
    QString str = "<foo><bar>kake</bar><bar>ja</bar></foo>";
    QBuffer buff;
    buff.setData((char*)str.utf16(), str.size()*sizeof(ushort));

    QXmlInputSource input(&buff);

    QXmlSimpleReader reader;
    QXmlDefaultHandler handler;
    reader.setContentHandler(&handler);

    QVERIFY(reader.parse(&input));
}

void tst_QXmlSimpleReader::inputFromSocket_data()
{
    QStringList files = findXmlFiles(QLatin1String("encodings"));
    QVERIFY(files.count() > 0);

    QTest::addColumn<QString>("file_name");

    foreach (const QString &file_name, files)
        QTest::newRow(file_name.toLatin1()) << file_name;
}

void tst_QXmlSimpleReader::inputFromSocket()
{
    QFETCH(QString, file_name);

    QTRY_VERIFY(server->listening);

    QTcpSocket sock;
    sock.connectToHost(QHostAddress::LocalHost, TEST_PORT);
    QVERIFY2(sock.waitForConnected(),
             qPrintable(QStringLiteral("Cannot connect on port ") + QString::number(TEST_PORT)
                        + QStringLiteral(": ") + sock.errorString()));

    sock.write(file_name.toLocal8Bit() + "\n");
    QVERIFY(sock.waitForBytesWritten());

    QXmlInputSource input(&sock);

    QXmlSimpleReader reader;
    QXmlDefaultHandler handler;
    reader.setContentHandler(&handler);

    QVERIFY(reader.parse(&input));

//    qDebug() << "tst_QXmlSimpleReader::inputFromSocket(): success" << file_name;
}

void tst_QXmlSimpleReader::preserveCharacterReferences() const
{
    class Handler : public QXmlDefaultHandler
    {
    public:
        virtual bool characters(const QString &chars)
        {
            received = chars;
            return true;
        }

        QString received;
    };

    {
        QByteArray input("<e>A&#160;&#160;&#160;&#160;A</e>");

        QBuffer buff(&input);
        QXmlInputSource source(&buff);

        Handler h;
        QXmlSimpleReader reader;
        reader.setContentHandler(&h);
        QVERIFY(reader.parse(&source, false));

        QCOMPARE(h.received, QLatin1Char('A') + QString(4, QChar(160)) + QLatin1Char('A'));
    }

    {
        QByteArray input("<e>&#160;&#160;&#160;&#160;</e>");

        QBuffer buff(&input);
        QXmlInputSource source(&buff);

        Handler h;
        QXmlSimpleReader reader;
        reader.setContentHandler(&h);
        QVERIFY(reader.parse(&source, false));

        QCOMPARE(h.received, QString(4, QChar(160)));
    }
}

void tst_QXmlSimpleReader::reportNamespace() const
{
    class Handler : public QXmlDefaultHandler
    {
    public:
        virtual bool startElement(const QString &namespaceURI,
                                  const QString &localName,
                                  const QString &qName,
                                  const QXmlAttributes &)
        {
            startNamespaceURI = namespaceURI;
            startLocalName = localName;
            startQName = qName;

            return true;
        }

        virtual bool endElement(const QString &namespaceURI,
                                const QString &localName,
                                const QString &qName)
        {
            endNamespaceURI = namespaceURI;
            endLocalName = localName;
            endQName = qName;

            return true;
        }

        QString startLocalName;
        QString startQName;
        QString startNamespaceURI;
        QString endLocalName;
        QString endQName;
        QString endNamespaceURI;
    };

    QXmlSimpleReader reader;
    Handler handler;
    reader.setContentHandler(&handler);

    QFETCH(QByteArray,  input);

    QBuffer buffer(&input);
    QVERIFY(buffer.open(QIODevice::ReadOnly));

    QXmlInputSource source(&buffer);
    QVERIFY(reader.parse(source));

    QFETCH(QString,     expectedQName);
    QFETCH(QString,     expectedLocalName);
    QFETCH(QString,     expectedNamespace);

    QCOMPARE(handler.startNamespaceURI, expectedNamespace);
    QCOMPARE(handler.startLocalName, expectedLocalName);
    QCOMPARE(handler.startQName, expectedQName);

    QCOMPARE(handler.endNamespaceURI, expectedNamespace);
    QCOMPARE(handler.endLocalName, expectedLocalName);
    QCOMPARE(handler.endQName, expectedQName);
}

void tst_QXmlSimpleReader::reportNamespace_data() const
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QString>("expectedQName");
    QTest::addColumn<QString>("expectedLocalName");
    QTest::addColumn<QString>("expectedNamespace");

    QTest::newRow("default ns") << QByteArray("<element xmlns='http://example.com/'/>")
                                << QString("element")
                                << QString("element")
                                << QString("http://example.com/");

    QTest::newRow("with prefix") << QByteArray("<p:element xmlns:p='http://example.com/'/>")
                                 << QString("p:element")
                                 << QString("element")
                                 << QString("http://example.com/");
}

QDomDocument tst_QXmlSimpleReader::fromByteArray(const QString &title, const QByteArray &ba, bool *ok)
{
    QDomDocument doc(title);
    *ok = doc.setContent(ba, true);
    return doc;
}

void tst_QXmlSimpleReader::roundtripWithNamespaces() const
{
    const char *const expected = "<element b:attr=\"value\" xmlns:a=\"http://www.example.com/A\" xmlns:b=\"http://www.example.com/B\" />\n";
    bool ok;

    {
        const char *const xml = "<element xmlns:b=\"http://www.example.com/B\" b:attr=\"value\" xmlns:a=\"http://www.example.com/A\"/>";

        const QDomDocument one(fromByteArray("document", xml, &ok));
        QVERIFY(ok);
        const QDomDocument two(fromByteArray("document2", one.toByteArray(2), &ok));
        QVERIFY(ok);

        QEXPECT_FAIL("", "Known problem, see 154573. The fix happens to break uic.", Abort);

        QCOMPARE(expected, one.toByteArray().constData());
        QCOMPARE(one.toByteArray(2).constData(), two.toByteArray(2).constData());
        QCOMPARE(two.toByteArray(2).constData(), two.toByteArray(2).constData());
    }

    {
        const char *const xml = "<element b:attr=\"value\" xmlns:b=\"http://www.example.com/B\" xmlns:a=\"http://www.example.com/A\"/>";

        const QDomDocument one(fromByteArray("document", xml, &ok));
        QVERIFY(ok);
        const QDomDocument two(fromByteArray("document2", one.toByteArray(2), &ok));
        QVERIFY(ok);

        QCOMPARE(expected, one.toByteArray().constData());
        QCOMPARE(one.toByteArray(2).constData(), two.toByteArray(2).constData());
        QCOMPARE(two.toByteArray(2).constData(), two.toByteArray(2).constData());
    }
}

class TestHandler : public QXmlDefaultHandler
{
public:
    TestHandler() :
        recursionCount(0)
    {
    }

    bool internalEntityDecl(const QString &name, const QString &value)
    {
        ++recursionCount;
        return QXmlDefaultHandler::internalEntityDecl(name, value);
    }

    int recursionCount;
};

void tst_QXmlSimpleReader::dtdRecursionLimit()
{
    QFile file("xmldocs/2-levels-nested-dtd.xml");
    QVERIFY(file.open(QIODevice::ReadOnly));
    QXmlSimpleReader xmlReader;
    {
        QXmlInputSource *source = new QXmlInputSource(&file);
        TestHandler handler;
        xmlReader.setDeclHandler(&handler);
        xmlReader.setErrorHandler(&handler);
        QVERIFY(!xmlReader.parse(source));
    }

    file.close();
    file.setFileName("xmldocs/1-levels-nested-dtd.xml");
    QVERIFY(file.open(QIODevice::ReadOnly));
    {
        QXmlInputSource *source = new QXmlInputSource(&file);
        TestHandler handler;
        xmlReader.setDeclHandler(&handler);
        xmlReader.setErrorHandler(&handler);
        QVERIFY(!xmlReader.parse(source));
        // The error wasn't because of the recursion limit being reached,
        // it was because the document is not valid.
        QVERIFY(handler.recursionCount < 2);
    }

    file.close();
    file.setFileName("xmldocs/internal-entity-polynomial-attribute.xml");
    QVERIFY(file.open(QIODevice::ReadOnly));
    {
        QXmlInputSource *source = new QXmlInputSource(&file);
        TestHandler handler;
        xmlReader.setDeclHandler(&handler);
        xmlReader.setErrorHandler(&handler);
        QVERIFY(!xmlReader.parse(source));
        QCOMPARE(handler.recursionCount, 2);
    }
}

QTEST_MAIN(tst_QXmlSimpleReader)
#include "tst_qxmlsimplereader.moc"
