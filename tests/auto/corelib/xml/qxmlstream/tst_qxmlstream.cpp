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


#include <QDirIterator>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QtTest/QtTest>
#include <QUrl>
#include <QXmlDefaultHandler>
#include <QXmlStreamReader>

#include "qc14n.h"

Q_DECLARE_METATYPE(QXmlStreamReader::ReadElementTextBehaviour)

static const char *const catalogFile = "XML-Test-Suite/xmlconf/finalCatalog.xml";
static const int expectedRunCount = 1646;
static const int expectedSkipCount = 532;

static inline int best(int a, int b)
{
    if (a < 0)
        return b;
    if (b < 0)
        return a;
    return qMin(a, b);
}

static inline int best(int a, int b, int c)
{
    if (a < 0)
        return best(b, c);
    if (b < 0)
        return best(a, c);
    if (c < 0)
        return best(a, b);
    return qMin(qMin(a, b), c);
}

/**
 *  Opens \a filename and returns content produced as per
 *  xmlconf/xmltest/canonxml.html.
 *
 *  \a docType is the DOCTYPE name that the returned output should
 *  have, if it doesn't already have one.
 */
static QByteArray makeCanonical(const QString &filename,
                                const QString &docType,
                                bool &hasError,
                                bool testIncremental = false)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly);

    QXmlStreamReader reader;

    QByteArray buffer;
    int bufferPos = 0;

    if (testIncremental)
        buffer = file.readAll();
    else
        reader.setDevice(&file);

    QByteArray outarray;
    QXmlStreamWriter writer(&outarray);

    forever {
        while (!reader.atEnd()) {
            reader.readNext();
            if (reader.isDTD()) {
                if (!reader.notationDeclarations().isEmpty()) {
                    QString dtd;
                    QTextStream writeDtd(&dtd);

                    writeDtd << "<!DOCTYPE ";
                    writeDtd << docType;
                    writeDtd << " [";
                    writeDtd << endl;
                    QMap<QString, QXmlStreamNotationDeclaration> sortedNotationDeclarations;
                    foreach (QXmlStreamNotationDeclaration notation, reader.notationDeclarations())
                        sortedNotationDeclarations.insert(notation.name().toString(), notation);
                    foreach (QXmlStreamNotationDeclaration notation, sortedNotationDeclarations.values()) {
                        writeDtd << "<!NOTATION ";
                        writeDtd << notation.name().toString();
                        if (notation.publicId().isEmpty()) {
                            writeDtd << " SYSTEM \'";
                            writeDtd << notation.systemId().toString();
                            writeDtd << "\'";
                        } else {
                            writeDtd << " PUBLIC \'";
                            writeDtd << notation.publicId().toString();
                            writeDtd << "\'";
                            if (!notation.systemId().isEmpty() ) {
                                writeDtd << " \'";
                                writeDtd << notation.systemId().toString();
                                writeDtd << "\'";
                            }
                        }
                        writeDtd << ">";
                        writeDtd << endl;
                    }

                    writeDtd << "]>";
                    writeDtd << endl;
                    writer.writeDTD(dtd);
                }
            } else if (reader.isStartElement()) {
                writer.writeStartElement(reader.namespaceUri().toString(), reader.name().toString());

                QMap<QString, QXmlStreamAttribute> sortedAttributes;
                foreach(QXmlStreamAttribute attribute, reader.attributes())
                    sortedAttributes.insert(attribute.name().toString(), attribute);
                foreach(QXmlStreamAttribute attribute, sortedAttributes.values())
                    writer.writeAttribute(attribute);
                writer.writeCharacters(QString()); // write empty string to avoid having empty xml tags
            } else if (reader.isCharacters()) {
                // make canonical

                QString text = reader.text().toString();
                int i = 0;
                int p = 0;
                while ((i = best(text.indexOf(QLatin1Char(10), p),
                                 text.indexOf(QLatin1Char(13), p),
                                 text.indexOf(QLatin1Char(9), p))) >= 0) {
                    writer.writeCharacters(text.mid(p, i - p));
                    writer.writeEntityReference(QString("#%1").arg(text.at(i).unicode()));
                    p = i + 1;
                }
                writer.writeCharacters(text.mid(p));
            } else if (reader.isStartDocument() || reader.isEndDocument() || reader.isComment()){
                // canonical does not want any of those
            } else if (reader.isProcessingInstruction() && reader.processingInstructionData().isEmpty()) {
                // for some reason canonical wants a space
                writer.writeProcessingInstruction(reader.processingInstructionTarget().toString(), QLatin1String(""));
            } else if (!reader.hasError()){
                writer.writeCurrentToken(reader);
            }
        }
        if (testIncremental && bufferPos < buffer.size()) {
            reader.addData(QByteArray(buffer.data() + (bufferPos++), 1));
        } else {
            break;
        }
    }

    if (reader.hasError()) {
        hasError = true;
        outarray += "ERROR:";
        outarray += reader.errorString().toLatin1();
    }
    else
        hasError = false;

    return outarray;
}

/**
 * \brief Returns the lexical QName of the document element in
 * \a document.
 *
 * It is assumed that \a document is a well-formed XML document.
 */
static QString documentElement(const QByteArray &document)
{
    QXmlStreamReader reader(document);

    while(!reader.atEnd())
    {
        if(reader.isStartElement())
            return reader.qualifiedName().toString();

        reader.readNext();
    }

    qFatal("The input %s didn't contain an element", document.constData());
    return QString();
}

/**
 * \brief Loads W3C's XML conformance test suite and runs it on QXmlStreamReader.
 *
 * Since this suite is fairly large, it runs the tests sequentially in order to not
 * have them all loaded into memory at once. In this way, the maximum memory usage stays
 * low, which means one can run valgrind on this test. However, the drawback is that
 * Qt Test's usual error reporting and testing mechanisms are slightly bypassed.
 *
 * Part of this code is a manual, ad-hoc implementation of xml:base.
 *
 * See \l {http://www.w3.org/XML/Test/} {Extensible Markup Language (XML) Conformance Test Suites}
 */
class TestSuiteHandler : public QXmlDefaultHandler
{
public:
    /**
     * The first string is the test ID, the second is
     * a description of what went wrong.
     */
    typedef QPair<QString, QString> GeneralFailure;

    /**
     * The string is the test ID.
     */
    QStringList successes;

    /**
     * The first value is the baseline, while the se
     */
    class MissedBaseline
    {
    public:
        MissedBaseline(const QString &aId,
                       const QByteArray &aExpected,
                       const QByteArray &aOutput) : id(aId),
                                                    expected(aExpected),
                                                    output(aOutput)
        {
            if (aId.isEmpty())
                qFatal("%s: aId must not be an empty string", Q_FUNC_INFO);
        }

        QString     id;
        QByteArray  expected;
        QByteArray  output;
    };

    QList<GeneralFailure> failures;
    QList<MissedBaseline> missedBaselines;

    /**
     * The count of how many tests that were run.
     */
    int runCount;

    int skipCount;

    /**
     * \a baseURI is the URI of where the catalog file resides.
     */
    TestSuiteHandler(const QUrl &baseURI) : runCount(0),
                                            skipCount(0)
    {
        if (!baseURI.isValid())
            qFatal("%s: baseURI must be valid", Q_FUNC_INFO);
        m_baseURI.push(baseURI);
    }

    virtual bool characters(const QString &chars)
    {
        m_ch = chars;
        return true;
    }

    virtual bool startElement(const QString &,
                              const QString &,
                              const QString &,
                              const QXmlAttributes &atts)
    {
        m_atts.push(atts);
        const int i = atts.index(QLatin1String("xml:base"));

        if(i != -1)
            m_baseURI.push(m_baseURI.top().resolved(atts.value(i)));

        return true;
    }

    virtual bool endElement(const QString &,
                            const QString &localName,
                            const QString &)
    {
        if(localName == QLatin1String("TEST"))
        {
            /* We don't want tests for XML 1.1.0, in fact). */
            if(m_atts.top().value(QString(), QLatin1String("VERSION")) == QLatin1String("1.1"))
            {
                ++skipCount;
                m_atts.pop();
                return true;
            }

            /* We don't want tests that conflict with the namespaces spec. Our parser is a
             * namespace-aware parser. */
            else if(m_atts.top().value(QString(), QLatin1String("NAMESPACE")) == QLatin1String("no"))
            {
                ++skipCount;
                m_atts.pop();
                return true;
            }

            const QString inputFilePath(m_baseURI.top().resolved(m_atts.top().value(QString(), QLatin1String("URI")))
                                                                .toLocalFile());
            const QString id(m_atts.top().value(QString(), QLatin1String("ID")));
            const QString type(m_atts.top().value(QString(), QLatin1String("TYPE")));

            QString expectedFilePath;
            const int index = m_atts.top().index(QString(), QLatin1String("OUTPUT"));

            if(index != -1)
            {
                expectedFilePath = m_baseURI.top().resolved(m_atts.top().value(QString(),
                                                            QLatin1String("OUTPUT"))).toLocalFile();
            }

            /* testcases.dtd: 'No parser should accept a "not-wf" testcase
             * unless it's a nonvalidating parser and the test contains
             * external entities that the parser doesn't read.'
             *
             * We also let this apply to "valid", "invalid" and "error" tests, although
             * I'm not fully sure this is correct. */
            const QString ents(m_atts.top().value(QString(), QLatin1String("ENTITIES")));
            m_atts.pop();

            if(ents == QLatin1String("both") ||
               ents == QLatin1String("general") ||
               ents == QLatin1String("parameter"))
            {
                ++skipCount;
                return true;
            }

            ++runCount;

            QFile inputFile(inputFilePath);
            if(!inputFile.open(QIODevice::ReadOnly))
            {
                failures.append(qMakePair(id, QString::fromLatin1("Failed to open input file %1").arg(inputFilePath)));
                return true;
            }

            if(type == QLatin1String("not-wf"))
            {
                if(isWellformed(&inputFile, ParseSinglePass))
                {
                     failures.append(qMakePair(id, QString::fromLatin1("Failed to flag %1 as not well-formed.")
                                                   .arg(inputFilePath)));

                     /* Exit, the incremental test will fail as well, no need to flood the output. */
                     return true;
                }
                else
                    successes.append(id);

                if(isWellformed(&inputFile, ParseIncrementally))
                {
                     failures.append(qMakePair(id, QString::fromLatin1("Failed to flag %1 as not well-formed with incremental parsing.")
                                                   .arg(inputFilePath)));
                }
                else
                    successes.append(id);

                return true;
            }

            QXmlStreamReader reader(&inputFile);

            /* See testcases.dtd which reads: 'Nonvalidating parsers
             * must also accept "invalid" testcases, but validating ones must reject them.' */
            if(type == QLatin1String("invalid") || type == QLatin1String("valid"))
            {
                QByteArray expected;
                QString docType;

                /* We only want to compare against a baseline when we have
                 * one. Some "invalid"-tests, for instance, doesn't have baselines. */
                if(!expectedFilePath.isEmpty())
                {
                    QFile expectedFile(expectedFilePath);

                    if(!expectedFile.open(QIODevice::ReadOnly))
                    {
                        failures.append(qMakePair(id, QString::fromLatin1("Failed to open baseline %1").arg(expectedFilePath)));
                        return true;
                    }

                    expected = expectedFile.readAll();
                    docType = documentElement(expected);
                }
                else
                    docType = QLatin1String("dummy");

                bool hasError = true;
                bool incremental = false;

                QByteArray input(makeCanonical(inputFilePath, docType, hasError, incremental));

                if (!hasError && !expectedFilePath.isEmpty() && input == expected)
                    input = makeCanonical(inputFilePath, docType, hasError, (incremental = true));

                if(hasError)
                    failures.append(qMakePair(id, QString::fromLatin1("Failed to parse %1%2")
                                              .arg(incremental?"(incremental run only) ":"")
                                              .arg(inputFilePath)));

                if(!expectedFilePath.isEmpty() && input != expected)
                {
                    missedBaselines.append(MissedBaseline(id, expected, input));
                    return true;
                }
                else
                {
                    successes.append(id);
                    return true;
                }
            }
            else if(type == QLatin1String("error"))
            {
                /* Not yet sure about this one. */
                // TODO
                return true;
            }
            else
            {
                qFatal("The input catalog is invalid.");
                return false;
            }
        }
        else if(localName == QLatin1String("TESTCASES") && m_atts.top().index(QLatin1String("xml:base")) != -1)
            m_baseURI.pop();

        m_atts.pop();

        return true;
    }

    enum ParseMode
    {
        ParseIncrementally,
        ParseSinglePass
    };

    static bool isWellformed(QIODevice *const inputFile, const ParseMode mode)
    {
        if (!inputFile)
            qFatal("%s: inputFile must be a valid QIODevice pointer", Q_FUNC_INFO);
        if (!inputFile->isOpen())
            qFatal("%s: inputFile must be opened by the caller", Q_FUNC_INFO);
        if (mode != ParseIncrementally && mode != ParseSinglePass)
            qFatal("%s: mode must be either ParseIncrementally or ParseSinglePass", Q_FUNC_INFO);

        if(mode == ParseIncrementally)
        {
            QXmlStreamReader reader;
            QByteArray buffer;
            int bufferPos = 0;

            buffer = inputFile->readAll();

            while(true)
            {
                while(!reader.atEnd())
                    reader.readNext();

                if(bufferPos < buffer.size())
                {
                    ++bufferPos;
                    reader.addData(QByteArray(buffer.data() + bufferPos, 1));
                }
                else
                    break;
            }

            return !reader.hasError();
        }
        else
        {
            QXmlStreamReader reader;
            reader.setDevice(inputFile);

            while(!reader.atEnd())
                reader.readNext();

            return !reader.hasError();
        }
    }

private:
    QStack<QXmlAttributes>  m_atts;
    QString                 m_ch;
    QStack<QUrl>            m_baseURI;
};

class tst_QXmlStream: public QObject
{
    Q_OBJECT
public:
    tst_QXmlStream() : m_handler(QUrl::fromLocalFile(QFINDTESTDATA(catalogFile)))
    {
    }

private slots:
    void initTestCase();
    void cleanupTestCase();
    void reportFailures() const;
    void reportFailures_data();
    void checkBaseline() const;
    void checkBaseline_data() const;
    void testReader() const;
    void testReader_data() const;
    void reportSuccess() const;
    void reportSuccess_data() const;
    void writerHangs() const;
    void writerAutoFormattingWithComments() const;
    void writerAutoFormattingWithTabs() const;
    void writerAutoFormattingWithProcessingInstructions() const;
    void writerAutoEmptyTags() const;
    void writeAttributesWithSpace() const;
    void addExtraNamespaceDeclarations();
    void setEntityResolver();
    void readFromQBuffer() const;
    void readFromQBufferInvalid() const;
    void readNextStartElement() const;
    void readElementText() const;
    void readElementText_data() const;
    void crashInUTF16Codec() const;
    void hasAttributeSignature() const;
    void hasAttribute() const;
    void writeWithCodec() const;
    void writeWithUtf8Codec() const;
    void writeWithUtf16Codec() const;
    void writeWithStandalone() const;
    void entitiesAndWhitespace_1() const;
    void entitiesAndWhitespace_2() const;
    void testFalsePrematureError() const;
    void garbageInXMLPrologDefaultCodec() const;
    void garbageInXMLPrologUTF8Explicitly() const;
    void clear() const;
    void checkCommentIndentation() const;
    void checkCommentIndentation_data() const;
    void crashInXmlStreamReader() const;
    void hasError() const;

private:
    static QByteArray readFile(const QString &filename);

    TestSuiteHandler m_handler;
};

void tst_QXmlStream::initTestCase()
{
    QFile file(QFINDTESTDATA(catalogFile));
    QVERIFY2(file.open(QIODevice::ReadOnly),
             qPrintable(QString::fromLatin1("Failed to open the test suite catalog; %1").arg(file.fileName())));

    QXmlInputSource source(&file);
    QXmlSimpleReader reader;
    reader.setContentHandler(&m_handler);

    QVERIFY(reader.parse(&source, false));
}

void tst_QXmlStream::cleanupTestCase()
{
    QFile::remove(QLatin1String("test.xml"));
}

void tst_QXmlStream::reportFailures() const
{
    QFETCH(bool, isError);
    QFETCH(QString, description);

    QVERIFY2(!isError, qPrintable(description));
}

void tst_QXmlStream::reportFailures_data()
{
    const int len = m_handler.failures.count();

    QTest::addColumn<bool>("isError");
    QTest::addColumn<QString>("description");

    /* We loop over all our failures(if any!), and output them such
     * that they appear in the Qt Test log. */
    for(int i = 0; i < len; ++i)
        QTest::newRow(m_handler.failures.at(i).first.toLatin1().constData()) << true << m_handler.failures.at(i).second;

    /* We need to add at least one column of test data, otherwise Qt Test complains. */
    if(len == 0)
        QTest::newRow("Whole test suite passed") << false << QString();

    /* We compare the test case counts to ensure that we've actually run test cases, that
     * the driver hasn't been broken or changed without updating the expected count, and
     * similar reasons. */
    QCOMPARE(m_handler.runCount, expectedRunCount);
    QCOMPARE(m_handler.skipCount, expectedSkipCount);
}

void tst_QXmlStream::checkBaseline() const
{
    QFETCH(bool, isError);
    QFETCH(QString, expected);
    QFETCH(QString, output);

    if(isError)
        QCOMPARE(output, expected);
}

void tst_QXmlStream::checkBaseline_data() const
{
    QTest::addColumn<bool>("isError");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<QString>("output");

    const int len = m_handler.missedBaselines.count();

    for(int i = 0; i < len; ++i)
    {
        const TestSuiteHandler::MissedBaseline &b = m_handler.missedBaselines.at(i);

        /* We indeed don't know what encoding the content is in so in some cases fromUtf8
         * is all wrong, but it's an acceptable guess for error reporting. */
        QTest::newRow(b.id.toLatin1().constData())
                << true
                << QString::fromUtf8(b.expected.constData())
                << QString::fromUtf8(b.output.constData());
    }

    if(len == 0)
        QTest::newRow("dummy") << false << QString() << QString();
}

void tst_QXmlStream::reportSuccess() const
{
    QFETCH(bool, isError);

    QVERIFY(!isError);
}

void tst_QXmlStream::reportSuccess_data() const
{
    QTest::addColumn<bool>("isError");

    const int len = m_handler.successes.count();

    for(int i = 0; i < len; ++i)
        QTest::newRow(qPrintable(QString("%1. %2").arg(i).arg(m_handler.successes.at(i)))) << false;

    if(len == 0)
        QTest::newRow("No test cases succeeded.") << true;
}

QByteArray tst_QXmlStream::readFile(const QString &filename)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly);

    QXmlStreamReader reader;

    reader.setDevice(&file);
    QByteArray outarray;
    QTextStream writer(&outarray);
    // We always want UTF-8, and not what the system picks up.
    writer.setCodec("UTF-8");

    while (!reader.atEnd()) {
        reader.readNext();
        writer << reader.tokenString() << "(";
        if (reader.isWhitespace())
            writer << " whitespace";
        if (reader.isCDATA())
            writer << " CDATA";
        if (reader.isStartDocument() && reader.isStandaloneDocument())
            writer << " standalone";
        if (!reader.text().isEmpty())
            writer << " text=\"" << reader.text().toString() << "\"";
        if (!reader.processingInstructionTarget().isEmpty())
            writer << " processingInstructionTarget=\"" << reader.processingInstructionTarget().toString() << "\"";
        if (!reader.processingInstructionData().isEmpty())
            writer << " processingInstructionData=\"" << reader.processingInstructionData().toString() << "\"";
        if (!reader.dtdName().isEmpty())
            writer << " dtdName=\"" << reader.dtdName().toString() << "\"";
        if (!reader.dtdPublicId().isEmpty())
            writer << " dtdPublicId=\"" << reader.dtdPublicId().toString() << "\"";
        if (!reader.dtdSystemId().isEmpty())
            writer << " dtdSystemId=\"" << reader.dtdSystemId().toString() << "\"";
        if (!reader.documentVersion().isEmpty())
            writer << " documentVersion=\"" << reader.documentVersion().toString() << "\"";
        if (!reader.documentEncoding().isEmpty())
            writer << " documentEncoding=\"" << reader.documentEncoding().toString() << "\"";
        if (!reader.name().isEmpty())
            writer << " name=\"" << reader.name().toString() << "\"";
        if (!reader.namespaceUri().isEmpty())
            writer << " namespaceUri=\"" << reader.namespaceUri().toString() << "\"";
        if (!reader.qualifiedName().isEmpty())
            writer << " qualifiedName=\"" << reader.qualifiedName().toString() << "\"";
        if (!reader.prefix().isEmpty())
            writer << " prefix=\"" << reader.prefix().toString() << "\"";
        if (reader.attributes().size()) {
            foreach(QXmlStreamAttribute attribute, reader.attributes()) {
                writer << endl << "    Attribute(";
                if (!attribute.name().isEmpty())
                    writer << " name=\"" << attribute.name().toString() << "\"";
                if (!attribute.namespaceUri().isEmpty())
                    writer << " namespaceUri=\"" << attribute.namespaceUri().toString() << "\"";
                if (!attribute.qualifiedName().isEmpty())
                    writer << " qualifiedName=\"" << attribute.qualifiedName().toString() << "\"";
                if (!attribute.prefix().isEmpty())
                    writer << " prefix=\"" << attribute.prefix().toString() << "\"";
                if (!attribute.value().isEmpty())
                    writer << " value=\"" << attribute.value().toString() << "\"";
                writer << " )" << endl;
            }
        }
        if (reader.namespaceDeclarations().size()) {
            foreach(QXmlStreamNamespaceDeclaration namespaceDeclaration, reader.namespaceDeclarations()) {
                writer << endl << "    NamespaceDeclaration(";
                if (!namespaceDeclaration.prefix().isEmpty())
                    writer << " prefix=\"" << namespaceDeclaration.prefix().toString() << "\"";
                if (!namespaceDeclaration.namespaceUri().isEmpty())
                    writer << " namespaceUri=\"" << namespaceDeclaration.namespaceUri().toString() << "\"";
                writer << " )" << endl;
            }
        }
        if (reader.notationDeclarations().size()) {
            foreach(QXmlStreamNotationDeclaration notationDeclaration, reader.notationDeclarations()) {
                writer << endl << "    NotationDeclaration(";
                if (!notationDeclaration.name().isEmpty())
                    writer << " name=\"" << notationDeclaration.name().toString() << "\"";
                if (!notationDeclaration.systemId().isEmpty())
                    writer << " systemId=\"" << notationDeclaration.systemId().toString() << "\"";
                if (!notationDeclaration.publicId().isEmpty())
                    writer << " publicId=\"" << notationDeclaration.publicId().toString() << "\"";
                writer << " )" << endl;
            }
        }
        if (reader.entityDeclarations().size()) {
            foreach(QXmlStreamEntityDeclaration entityDeclaration, reader.entityDeclarations()) {
                writer << endl << "    EntityDeclaration(";
                if (!entityDeclaration.name().isEmpty())
                    writer << " name=\"" << entityDeclaration.name().toString() << "\"";
                if (!entityDeclaration.notationName().isEmpty())
                    writer << " notationName=\"" << entityDeclaration.notationName().toString() << "\"";
                if (!entityDeclaration.systemId().isEmpty())
                    writer << " systemId=\"" << entityDeclaration.systemId().toString() << "\"";
                if (!entityDeclaration.publicId().isEmpty())
                    writer << " publicId=\"" << entityDeclaration.publicId().toString() << "\"";
                if (!entityDeclaration.value().isEmpty())
                    writer << " value=\"" << entityDeclaration.value().toString() << "\"";
                writer << " )" << endl;
            }
        }
        writer << " )" << endl;
    }
    if (reader.hasError())
        writer << "ERROR: " << reader.errorString() << endl;
    return outarray;
}

void tst_QXmlStream::testReader() const
{
    QFETCH(QString, xml);
    QFETCH(QString, ref);
    QFile file(ref);
    if (!file.exists()) {
        QByteArray reference = readFile(xml);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(reference);
        file.close();
    } else {
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QString reference = QString::fromUtf8(file.readAll());
        QString qxmlstream = QString::fromUtf8(readFile(xml));
        QCOMPARE(qxmlstream, reference);
    }
}

void tst_QXmlStream::testReader_data() const
{
    QTest::addColumn<QString>("xml");
    QTest::addColumn<QString>("ref");
    QDir dir;
    dir.cd(QFINDTESTDATA("data/"));
    foreach(QString filename , dir.entryList(QStringList() << "*.xml")) {
        QString reference =  QFileInfo(filename).baseName() + ".ref";
        QTest::newRow(dir.filePath(filename).toLatin1().data()) << dir.filePath(filename) << dir.filePath(reference);
    }
}

void tst_QXmlStream::addExtraNamespaceDeclarations()
{
    const char *data = "<bla><undeclared:foo/><undeclared_too:foo/></bla>";
    {
        QXmlStreamReader xml(data);
        while (!xml.atEnd()) {
            xml.readNext();
        }
        QVERIFY2(xml.hasError(), "namespaces undeclared");
    }
    {
        QXmlStreamReader xml(data);
        xml.addExtraNamespaceDeclaration(QXmlStreamNamespaceDeclaration("undeclared", "blabla"));
        xml.addExtraNamespaceDeclaration(QXmlStreamNamespaceDeclaration("undeclared_too", "foofoo"));
        while (!xml.atEnd()) {
            xml.readNext();
        }
        QVERIFY2(!xml.hasError(), xml.errorString().toLatin1().constData());
    }
}


class EntityResolver : public QXmlStreamEntityResolver {
public:
    QString resolveUndeclaredEntity(const QString &name) {
        static int count = 0;
        return name.toUpper() + QString::number(++count);
    }
};
void tst_QXmlStream::setEntityResolver()
{
    const char *data = "<bla foo=\"&undeclared;\">&undeclared_too;</bla>";
    {
        QXmlStreamReader xml(data);
        while (!xml.atEnd()) {
            xml.readNext();
        }
        QVERIFY2(xml.hasError(), "undeclared entities");
    }
    {
        QString foo;
        QString bla_text;
        QXmlStreamReader xml(data);
        EntityResolver resolver;
        xml.setEntityResolver(&resolver);
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement())
                foo = xml.attributes().value("foo").toString();
            if (xml.isCharacters())
                bla_text += xml.text().toString();
        }
        QVERIFY2(!xml.hasError(), xml.errorString().toLatin1().constData());
        QCOMPARE(foo, QLatin1String("UNDECLARED1"));
        QCOMPARE(bla_text, QLatin1String("UNDECLARED_TOO2"));
    }
}

void tst_QXmlStream::testFalsePrematureError() const
{
    const char *illegal_start = "illegal<sta";
    const char *legal_start = "<sta";
    const char* end = "rt/>";
    {
        QXmlStreamReader xml("");
        while (!xml.atEnd()) {
            xml.readNext();
        }
        QVERIFY(xml.error() == QXmlStreamReader::PrematureEndOfDocumentError);
        QCOMPARE(xml.errorString(), QLatin1String("Premature end of document."));
        xml.addData(legal_start);
        while (!xml.atEnd()) {
            xml.readNext();
        }
        QVERIFY(xml.error() == QXmlStreamReader::PrematureEndOfDocumentError);
        QCOMPARE(xml.errorString(), QLatin1String("Premature end of document."));
        xml.addData(end);
        while (!xml.atEnd()) {
            xml.readNext();
        }
        QVERIFY(!xml.hasError());
    }
    {
        QXmlStreamReader xml(illegal_start);
        while (!xml.atEnd()) {
            xml.readNext();
        }
        QVERIFY(xml.hasError());
        QCOMPARE(xml.errorString(), QLatin1String("Start tag expected."));
        QVERIFY(xml.error() == QXmlStreamReader::NotWellFormedError);
    }
}

// Regression test for crash due to using empty QStack.
void tst_QXmlStream::writerHangs() const
{
    QFile file("test.xml");

    QVERIFY(file.open(QIODevice::WriteOnly));

    QXmlStreamWriter  writer(&file);
    double radius = 4.0;
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeEmptyElement("circle");
    writer.writeAttribute("radius", QString::number(radius));
    writer.writeEndElement();
    writer.writeEndDocument();
}

void tst_QXmlStream::writerAutoFormattingWithComments() const
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);

    QXmlStreamWriter writer(&buffer);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeComment("This is a comment");
    writer.writeEndDocument();
    const char *str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!--This is a comment-->\n";
    QCOMPARE(buffer.buffer().data(), str);
}

void tst_QXmlStream::writerAutoFormattingWithTabs() const
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);


    QXmlStreamWriter writer(&buffer);
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(-1);
    QCOMPARE(writer.autoFormattingIndent(), -1);
    writer.writeStartDocument();
    writer.writeStartElement("A");
    writer.writeStartElement("B");
    writer.writeEndDocument();
    const char *str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<A>\n\t<B/>\n</A>\n";
    QCOMPARE(buffer.buffer().data(), str);
}

void tst_QXmlStream::writerAutoFormattingWithProcessingInstructions() const
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);

    QXmlStreamWriter writer(&buffer);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeProcessingInstruction("B", "C");
    writer.writeStartElement("A");
    writer.writeEndElement();
    writer.writeEndDocument();
    const char *str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<?B C?>\n<A/>\n";
    QCOMPARE(buffer.buffer().data(), str);
}

void tst_QXmlStream::writeAttributesWithSpace() const
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);


    QXmlStreamWriter writer(&buffer);
    writer.writeStartDocument();
    writer.writeEmptyElement("A");
    writer.writeAttribute("attribute", QStringLiteral("value") + QChar(QChar::Nbsp));
    writer.writeEndDocument();
    QString s = QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><A attribute=\"value%1\"/>\n").arg(QChar(QChar::Nbsp));
    QCOMPARE(buffer.buffer().data(), s.toUtf8().data());
}

void tst_QXmlStream::writerAutoEmptyTags() const
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);


    QXmlStreamWriter writer(&buffer);

    writer.writeStartDocument();

    writer.writeStartElement("Hans");
    writer.writeAttribute("key", "value");
    writer.writeEndElement();

    writer.writeStartElement("Hans");
    writer.writeAttribute("key", "value");
    writer.writeEmptyElement("Leer");
    writer.writeAttribute("key", "value");
    writer.writeEndElement();

    writer.writeStartElement("Hans");
    writer.writeAttribute("key", "value");
    writer.writeCharacters("stuff");
    writer.writeEndElement();

    writer.writeEndDocument();

    QString s = QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><Hans key=\"value\"/><Hans key=\"value\"><Leer key=\"value\"/></Hans><Hans key=\"value\">stuff</Hans>\n");
    QCOMPARE(buffer.buffer().data(), s.toUtf8().data());
}

void tst_QXmlStream::readFromQBuffer() const
{
    QByteArray in("<e/>");
    QBuffer buffer(&in);
    QVERIFY(buffer.open(QIODevice::ReadOnly));

    QXmlStreamReader reader(&buffer);

    while(!reader.atEnd())
    {
        reader.readNext();
    }

    QVERIFY(!reader.hasError());
}

void tst_QXmlStream::readFromQBufferInvalid() const
{
    QByteArray in("<e/><e/>");
    QBuffer buffer(&in);
    QVERIFY(buffer.open(QIODevice::ReadOnly));

    QXmlStreamReader reader(&buffer);

    while(!reader.atEnd())
    {
        reader.readNext();
    }

    QVERIFY(reader.hasError());
}

void tst_QXmlStream::readNextStartElement() const
{
    QLatin1String in("<?xml version=\"1.0\"?><A><!-- blah --><B><C/></B><B attr=\"value\"/>text</A>");
    QXmlStreamReader reader(in);

    QVERIFY(reader.readNextStartElement());
    QVERIFY(reader.isStartElement() && reader.name() == "A");

    int amountOfB = 0;
    while (reader.readNextStartElement()) {
        QVERIFY(reader.isStartElement() && reader.name() == "B");
        ++amountOfB;
        reader.skipCurrentElement();
    }

    QCOMPARE(amountOfB, 2);
}

void tst_QXmlStream::readElementText() const
{
    QFETCH(QXmlStreamReader::ReadElementTextBehaviour, behaviour);
    QFETCH(QString, input);
    QFETCH(QString, expected);

    QXmlStreamReader reader(input);

    QVERIFY(reader.readNextStartElement());
    QCOMPARE(reader.readElementText(behaviour), expected);
}

void tst_QXmlStream::readElementText_data() const
{
    QTest::addColumn<QXmlStreamReader::ReadElementTextBehaviour>("behaviour");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QString validInput("<p>He was <em>never</em> going to admit<!-- TODO: rephrase --> his mistake.</p>");
    QString invalidInput("<p>invalid...<p>");
    QString invalidOutput("invalid...");

    QTest::newRow("ErrorOnUnexpectedElement")
            << QXmlStreamReader::ErrorOnUnexpectedElement
            << validInput << QString("He was ");

    QTest::newRow("IncludeChildElements")
            << QXmlStreamReader::IncludeChildElements
            << validInput << QString("He was never going to admit his mistake.");

    QTest::newRow("SkipChildElements")
            << QXmlStreamReader::SkipChildElements
            << validInput << QString("He was  going to admit his mistake.");

    QTest::newRow("ErrorOnUnexpectedElement Invalid")
            << QXmlStreamReader::ErrorOnUnexpectedElement
            << invalidInput << invalidOutput;

    QTest::newRow("IncludeChildElements Invalid")
            << QXmlStreamReader::IncludeChildElements
            << invalidInput << invalidOutput;

    QTest::newRow("SkipChildElements Invalid")
            << QXmlStreamReader::SkipChildElements
            << invalidInput << invalidOutput;
}

void tst_QXmlStream::crashInUTF16Codec() const
{
    QEventLoop eventLoop;

    QNetworkAccessManager networkManager;
    QNetworkRequest request(QUrl::fromLocalFile(QFINDTESTDATA("data/051reduced.xml")));
    QNetworkReply *const reply = networkManager.get(request);
    eventLoop.connect(reply, SIGNAL(finished()), SLOT(quit()));

    QCOMPARE(eventLoop.exec(), 0);

    QXmlStreamReader reader(reply);
    while(!reader.atEnd())
    {
        reader.readNext();
        continue;
    }

    QVERIFY(!reader.hasError());
}

/*
  In addition to Qt Test's flags, one can specify "-c <filename>" and have that file output in its canonical form.
*/
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if (argc == 3 && QByteArray(argv[1]).startsWith("-c")) {
        // output canonical only
        bool error = false;
        QByteArray canonical = makeCanonical(argv[2], "doc", error);
        QTextStream myStdOut(stdout);
        myStdOut << canonical << endl;
        exit(0);
    }

    tst_QXmlStream tc;
    return QTest::qExec(&tc, argc, argv);
}

void tst_QXmlStream::hasAttributeSignature() const
{
    /* These functions should be const so invoke all
     * of them on a const object. */
    const QXmlStreamAttributes atts;
    atts.hasAttribute(QLatin1String("localName"));
    atts.hasAttribute(QString::fromLatin1("localName"));
    atts.hasAttribute(QString::fromLatin1("http://example.com/"), QLatin1String("localName"));

    /* The input arguments should be const references, not mutable references
     * so pass const references. */
    const QLatin1String latin1StringLocalName(QLatin1String("localName"));
    const QString qStringLocalname(QLatin1String("localName"));
    const QString namespaceURI(QLatin1String("http://example.com/"));

    /* QLatin1String overload. */
    atts.hasAttribute(latin1StringLocalName);

    /* QString overload. */
    atts.hasAttribute(latin1StringLocalName);

    /* namespace/local name overload. */
    atts.hasAttribute(namespaceURI, qStringLocalname);
}

void tst_QXmlStream::hasAttribute() const
{
    QXmlStreamReader reader(QLatin1String("<e xmlns:p='http://example.com/2' xmlns='http://example.com/' "
                                          "attr1='value' attr2='value2' p:attr3='value3' emptyAttr=''><noAttributes/></e>"));

    QCOMPARE(reader.readNext(), QXmlStreamReader::StartDocument);
    QCOMPARE(reader.readNext(), QXmlStreamReader::StartElement);
    const QXmlStreamAttributes &atts = reader.attributes();

    /* QLatin1String overload. */
    QVERIFY(atts.hasAttribute(QLatin1String("attr1")));
    QVERIFY(atts.hasAttribute(QLatin1String("attr2")));
    QVERIFY(atts.hasAttribute(QLatin1String("p:attr3")));
    QVERIFY(atts.hasAttribute(QLatin1String("emptyAttr")));
    QVERIFY(!atts.hasAttribute(QLatin1String("DOESNOTEXIST")));

    /* Test with an empty & null namespaces. */
    QVERIFY(atts.hasAttribute(QString(), QLatin1String("attr2"))); /* A null string. */
    QVERIFY(atts.hasAttribute(QLatin1String(""), QLatin1String("attr2"))); /* An empty string. */

    /* QString overload. */
    QVERIFY(atts.hasAttribute(QString::fromLatin1("attr1")));
    QVERIFY(atts.hasAttribute(QString::fromLatin1("attr2")));
    QVERIFY(atts.hasAttribute(QString::fromLatin1("p:attr3")));
    QVERIFY(atts.hasAttribute(QString::fromLatin1("emptyAttr")));
    QVERIFY(!atts.hasAttribute(QString::fromLatin1("DOESNOTEXIST")));

    /* namespace/local name overload. */
    QVERIFY(atts.hasAttribute(QString(), QString::fromLatin1("attr1")));
    /* Attributes do not pick up the default namespace. */
    QVERIFY(!atts.hasAttribute(QLatin1String("http://example.com/"), QString::fromLatin1("attr1")));
    QVERIFY(atts.hasAttribute(QLatin1String("http://example.com/2"), QString::fromLatin1("attr3")));
    QVERIFY(atts.hasAttribute(QString(), QString::fromLatin1("emptyAttr")));
    QVERIFY(!atts.hasAttribute(QLatin1String("http://example.com/2"), QString::fromLatin1("DOESNOTEXIST")));
    QVERIFY(!atts.hasAttribute(QLatin1String("WRONG_NAMESPACE"), QString::fromLatin1("attr3")));

    /* Invoke on an QXmlStreamAttributes that has no attributes at all. */
    QCOMPARE(reader.readNext(), QXmlStreamReader::StartElement);

    const QXmlStreamAttributes &atts2 = reader.attributes();
    QVERIFY(atts2.isEmpty());

    /* QLatin1String overload. */
    QVERIFY(!atts.hasAttribute(QLatin1String("arbitraryName")));

    /* QString overload. */
    QVERIFY(!atts.hasAttribute(QString::fromLatin1("arbitraryName")));

    /* namespace/local name overload. */
    QVERIFY(!atts.hasAttribute(QLatin1String("http://example.com/"), QString::fromLatin1("arbitraryName")));

    while(!reader.atEnd())
        reader.readNext();

    QVERIFY(!reader.hasError());
}


void tst_QXmlStream::writeWithCodec() const
{
    QByteArray outarray;
    QXmlStreamWriter writer(&outarray);
    writer.setAutoFormatting(true);

    QTextCodec *codec = QTextCodec::codecForName("ISO 8859-15");
    QVERIFY(codec);
    writer.setCodec(codec);

    const char *latin2 = "h\xe9 h\xe9";
    const QString string = codec->toUnicode(latin2);


    writer.writeStartDocument("1.0");

    writer.writeTextElement("foo", string);
    writer.writeEndElement();
    writer.writeEndDocument();

    QVERIFY(outarray.contains(latin2));
    QVERIFY(outarray.contains(codec->name()));
}

void tst_QXmlStream::writeWithUtf8Codec() const
{
    QByteArray outarray;
    QXmlStreamWriter writer(&outarray);

    QTextCodec *codec = QTextCodec::codecForMib(106); // utf-8
    QVERIFY(codec);
    writer.setCodec(codec);

    writer.writeStartDocument("1.0");
    static const char begin[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    QVERIFY(outarray.startsWith(begin));
}

void tst_QXmlStream::writeWithUtf16Codec() const
{
    QByteArray outarray;
    QXmlStreamWriter writer(&outarray);

    QTextCodec *codec = QTextCodec::codecForMib(1014); // utf-16LE
    QVERIFY(codec);
    writer.setCodec(codec);

    writer.writeStartDocument("1.0");
    static const char begin[] = "<?xml version=\"1.0\" encoding=\"UTF-16";  // skip potential "LE" suffix
    const int count = sizeof(begin) - 1;    // don't include 0 terminator
    QByteArray begin_UTF16;
    begin_UTF16.reserve(2*(count));
    for (int i = 0; i < count; ++i) {
        begin_UTF16.append(begin[i]);
        begin_UTF16.append((char)'\0');
    }
    QVERIFY(outarray.startsWith(begin_UTF16));
}

void tst_QXmlStream::writeWithStandalone() const
{
    {
        QByteArray outarray;
        QXmlStreamWriter writer(&outarray);
        writer.setAutoFormatting(true);
        writer.writeStartDocument("1.0", true);
        writer.writeEndDocument();
        const char *ref = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
        QCOMPARE(outarray.constData(), ref);
    }
    {
        QByteArray outarray;
        QXmlStreamWriter writer(&outarray);
        writer.setAutoFormatting(true);
        writer.writeStartDocument("1.0", false);
        writer.writeEndDocument();
        const char *ref = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
        QCOMPARE(outarray.constData(), ref);
    }
}

void tst_QXmlStream::entitiesAndWhitespace_1() const
{
    QXmlStreamReader reader(QLatin1String("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\"><test>&extEnt;</test>"));

    int entityCount = 0;
    int characterCount = 0;
    while(!reader.atEnd())
    {
        QXmlStreamReader::TokenType token = reader.readNext();
        switch(token)
        {
            case QXmlStreamReader::Characters:
                characterCount++;
                break;
            case QXmlStreamReader::EntityReference:
                entityCount++;
                break;
            default:
                ;
        }
    }

    QCOMPARE(entityCount, 1);
    QCOMPARE(characterCount, 0);
    QVERIFY(!reader.hasError());
}

void tst_QXmlStream::entitiesAndWhitespace_2() const
{
    QXmlStreamReader reader(QLatin1String("<test>&extEnt;</test>"));

    int entityCount = 0;
    int characterCount = 0;
    while(!reader.atEnd())
    {
        QXmlStreamReader::TokenType token = reader.readNext();
        switch(token)
        {
            case QXmlStreamReader::Characters:
                characterCount++;
                break;
            case QXmlStreamReader::EntityReference:
                entityCount++;
                break;
            default:
                ;
        }
    }

    QCOMPARE(entityCount, 0);
    QCOMPARE(characterCount, 0);
    QVERIFY(reader.hasError());
}

void tst_QXmlStream::garbageInXMLPrologDefaultCodec() const
{
    QBuffer out;
    QVERIFY(out.open(QIODevice::ReadWrite));

    QXmlStreamWriter writer (&out);
    writer.writeStartDocument();
    writer.writeEmptyElement("Foo");
    writer.writeEndDocument();

    QCOMPARE(out.data(), QByteArray("<?xml version=\"1.0\" encoding=\"UTF-8\"?><Foo/>\n"));
}

void tst_QXmlStream::garbageInXMLPrologUTF8Explicitly() const
{
    QBuffer out;
    QVERIFY(out.open(QIODevice::ReadWrite));

    QXmlStreamWriter writer (&out);
    writer.setCodec("UTF-8");
    writer.writeStartDocument();
    writer.writeEmptyElement("Foo");
    writer.writeEndDocument();

    QCOMPARE(out.data(), QByteArray("<?xml version=\"1.0\" encoding=\"UTF-8\"?><Foo/>\n"));
}

void tst_QXmlStream::clear() const
{
    QString xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><body></body>";
    QXmlStreamReader reader;

    reader.addData(xml);
    while (!reader.atEnd()) {
        reader.readNext();
    }
    QCOMPARE(reader.tokenType(), QXmlStreamReader::EndDocument);

    reader.clear();
    reader.addData(xml);
    while (!reader.atEnd()) {
        reader.readNext();
    }
    QCOMPARE(reader.tokenType(), QXmlStreamReader::EndDocument);


    // now we stop in the middle to check whether clear really works
    reader.clear();
    reader.addData(xml);
    reader.readNext();
    reader.readNext();
    QCOMPARE(reader.tokenType(), QXmlStreamReader::StartElement);

    // and here the final read
    reader.clear();
    reader.addData(xml);
    while (!reader.atEnd()) {
        reader.readNext();
    }
    QCOMPARE(reader.tokenType(), QXmlStreamReader::EndDocument);
}

void tst_QXmlStream::checkCommentIndentation_data() const
{

    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedOutput");

    QString simpleInput = "<a><!-- bla --></a>";
    QString simpleOutput = "<?xml version=\"1.0\"?>\n"
                           "<a>\n"
                           "   <!-- bla -->\n"
                           "</a>\n";
    QTest::newRow("simple-comment") << simpleInput << simpleOutput;

    QString advancedInput = "<a><!-- bla --><!-- bla --><b><!-- bla --><c><!-- bla --></c><!-- bla --></b></a>";
    QString advancedOutput = "<?xml version=\"1.0\"?>\n"
                           "<a>\n"
                           "   <!-- bla -->\n"
                           "   <!-- bla -->\n"
                           "   <b>\n"
                           "      <!-- bla -->\n"
                           "      <c>\n"
                           "         <!-- bla -->\n"
                           "      </c>\n"
                           "      <!-- bla -->\n"
                           "   </b>\n"
                           "</a>\n";
    QTest::newRow("advanced-comment") << advancedInput << advancedOutput;
}

void tst_QXmlStream::checkCommentIndentation() const
{
    QFETCH(QString, input);
    QFETCH(QString, expectedOutput);
    QString output;
    QXmlStreamReader reader(input);
    QXmlStreamWriter writer(&output);
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(3);

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.error()) {
            QFAIL("error reading XML input");
        } else {
            writer.writeCurrentToken(reader);
        }
    }
    QCOMPARE(output, expectedOutput);
}

// This is a regression test for QTBUG-9196, where the series of tags used
// in the test caused a crash in the XML stream reader.
void tst_QXmlStream::crashInXmlStreamReader() const
{
    QByteArray ba("<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>"
                  "<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a></a>");
    QXmlStreamReader xml(ba);
    while (!xml.atEnd()) {
         xml.readNext();
    }
}

class FakeBuffer : public QBuffer
{
protected:
    qint64 writeData(const char *c, qint64 i)
    {
        qint64 ai = qMin(m_capacity, i);
        m_capacity -= ai;
        return ai ? QBuffer::writeData(c, ai) : 0;
    }
public:
    void setCapacity(int capacity) { m_capacity = capacity; }
private:
    qint64 m_capacity;
};

void tst_QXmlStream::hasError() const
{
    {
        FakeBuffer fb;
        QVERIFY(fb.open(QBuffer::ReadWrite));
        fb.setCapacity(1000);
        QXmlStreamWriter writer(&fb);
        writer.writeStartDocument();
        writer.writeEndDocument();
        QVERIFY(!writer.hasError());
        QCOMPARE(fb.data(), QByteArray("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"));
    }

    {
        // Failure caused by write(QString)
        FakeBuffer fb;
        QVERIFY(fb.open(QBuffer::ReadWrite));
        const QByteArray expected = QByteArrayLiteral("<?xml version=\"");
        fb.setCapacity(expected.size());
        QXmlStreamWriter writer(&fb);
        writer.writeStartDocument();
        QVERIFY(writer.hasError());
        QCOMPARE(fb.data(), expected);
    }

    {
        // Failure caused by write(char *)
        FakeBuffer fb;
        QVERIFY(fb.open(QBuffer::ReadWrite));
        const QByteArray expected = QByteArrayLiteral("<?xml version=\"1.0");
        fb.setCapacity(expected.size());
        QXmlStreamWriter writer(&fb);
        writer.writeStartDocument();
        QVERIFY(writer.hasError());
        QCOMPARE(fb.data(), expected);
    }

    {
        // Failure caused by write(QStringRef)
        FakeBuffer fb;
        QVERIFY(fb.open(QBuffer::ReadWrite));
        const QByteArray expected = QByteArrayLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?><test xmlns:");
        fb.setCapacity(expected.size());
        QXmlStreamWriter writer(&fb);
        writer.writeStartDocument();
        writer.writeStartElement("test");
        writer.writeNamespace("http://foo.bar", "foo");
        QVERIFY(writer.hasError());
        QCOMPARE(fb.data(), expected);
    }

    {
        // Refusal to write after 1st failure
        FakeBuffer fb;
        QVERIFY(fb.open(QBuffer::ReadWrite));
        fb.setCapacity(10);
        QXmlStreamWriter writer(&fb);
        writer.writeStartDocument();
        QVERIFY(writer.hasError());
        QCOMPARE(fb.data(), QByteArray("<?xml vers"));
        fb.setCapacity(1000);
        writer.writeStartElement("test"); // literal & qstring
        writer.writeNamespace("http://foo.bar", "foo"); // literal & qstringref
        QVERIFY(writer.hasError());
        QCOMPARE(fb.data(), QByteArray("<?xml vers"));
    }

}

#include "tst_qxmlstream.moc"
// vim: et:ts=4:sw=4:sts=4
