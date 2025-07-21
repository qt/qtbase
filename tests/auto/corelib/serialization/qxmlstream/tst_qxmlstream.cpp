// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QDirIterator>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTest>
#include <QtTest/private/qcomparisontesthelper_p.h>
#include <QUrl>
#include <QVarLengthArray>
#include <QXmlStreamReader>
#include <QBuffer>
#include <QStack>
#include <private/qzipreader_p.h>

#include "qc14n.h"

using namespace Qt::StringLiterals;

Q_DECLARE_METATYPE(QXmlStreamReader::ReadElementTextBehaviour)

static const char *const catalogFile = "XML-Test-Suite/xmlconf/finalCatalog.xml";
static const int expectedRunCount = 1646;
static const int expectedSkipCount = 532;
static const char *const xmlTestsuiteDir = "XML-Test-Suite";
static const char *const xmlconfDir = "XML-Test-Suite/xmlconf/";
static const char *const xmlDatasetName = "xmltest";
static const char *const updateFilesDir = "xmltest_updates";
static const char *const destinationFolder = "/valid/sa/out/";

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

// copied from tst_qmake.cpp
static void copyDir(const QString &sourceDirPath, const QString &targetDirPath)
{
    QDir currentDir;
    QDirIterator dit(sourceDirPath, QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);
    while (dit.hasNext()) {
        dit.next();
        const QString targetPath = targetDirPath + QLatin1Char('/') + dit.fileName();
        currentDir.mkpath(targetPath);
        copyDir(dit.filePath(), targetPath);
    }

    QDirIterator fit(sourceDirPath, QDir::Files | QDir::Hidden);
    while (fit.hasNext()) {
        fit.next();
        const QString targetPath = targetDirPath + QLatin1Char('/') + fit.fileName();
        QFile::remove(targetPath);  // allowed to fail
        QFile src(fit.filePath());
        QVERIFY2(src.copy(targetPath), qPrintable(src.errorString()));
    }
}

template <typename C>
const C sorted_by_name(C c) { // return by const value so we can feed directly into range-for loops below
    using T = typename C::value_type;
    auto byName = [](const T &lhs, const T &rhs) {
        return lhs.name() < rhs.name();
    };
    std::sort(c.begin(), c.end(), byName);
    return c;
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
    if (!file.open(QIODevice::ReadOnly))
        qFatal("Could not open file %s", qPrintable(filename));
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
                const auto notationDeclarations = reader.notationDeclarations();
                if (!notationDeclarations.isEmpty()) {
                    QString dtd;
                    QTextStream writeDtd(&dtd);

                    writeDtd << "<!DOCTYPE ";
                    writeDtd << docType;
                    writeDtd << " [";
                    writeDtd << Qt::endl;
                    for (const QXmlStreamNotationDeclaration &notation : sorted_by_name(notationDeclarations)) {
                        writeDtd << "<!NOTATION ";
                        writeDtd << notation.name().toString();
                        if (notation.publicId().isEmpty()) {
                            writeDtd << " SYSTEM \'";
                            writeDtd << notation.systemId().toString();
                            writeDtd << '\'';
                        } else {
                            writeDtd << " PUBLIC \'";
                            writeDtd << notation.publicId().toString();
                            writeDtd << "\'";
                            if (!notation.systemId().isEmpty() ) {
                                writeDtd << " \'";
                                writeDtd << notation.systemId().toString();
                                writeDtd << '\'';
                            }
                        }
                        writeDtd << '>';
                        writeDtd << Qt::endl;
                    }

                    writeDtd << "]>";
                    writeDtd << Qt::endl;
                    writer.writeDTD(dtd);
                }
            } else if (reader.isStartElement()) {
                writer.writeStartElement(reader.namespaceUri().toString(), reader.name().toString());
                for (const QXmlStreamAttribute &attribute : sorted_by_name(reader.attributes()))
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
                    writer.writeEntityReference(QLatin1Char('#') + QString::number(text.at(i).unicode()));
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
class TestSuiteHandler
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
        friend class QList<MissedBaseline>;
        MissedBaseline() {} // for QList, don't use
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

        void swap(MissedBaseline &other) noexcept
        {
            qSwap(id, other.id);
            qSwap(expected, other.expected);
            qSwap(output, other.output);
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

    bool runTests(QFile *file)
    {
        QXmlStreamReader reader(file);
        while (!reader.atEnd() && !reader.hasError()) {
            reader.readNext();

            if (reader.isStartElement() && !startElement(reader.attributes()))
                return false;

            if (reader.isEndElement() && !endElement(reader.name().toString()))
                return false;
        }
        return !reader.hasError();
    }

    bool startElement(const QXmlStreamAttributes &atts)
    {
        m_atts.push(atts);

        const auto attr = atts.value(QLatin1String("xml:base"));
        if (!attr.isEmpty())
            m_baseURI.push(m_baseURI.top().resolved(attr.toString()));

        return true;
    }

    bool endElement(const QString &localName)
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

            const QString inputFilePath(
                    m_baseURI.top()
                            .resolved(
                                    m_atts.top().value(QString(), QLatin1String("URI")).toString())
                            .toLocalFile());
            const QString id(m_atts.top().value(QString(), QLatin1String("ID")).toString());
            const QString type(m_atts.top().value(QString(), QLatin1String("TYPE")).toString());

            QString expectedFilePath;

            const auto attr = m_atts.top().value(QString(), QLatin1String("OUTPUT"));
            if (!attr.isEmpty())
                expectedFilePath = m_baseURI.top().resolved(attr.toString()).toLocalFile();

            /* testcases.dtd: 'No parser should accept a "not-wf" testcase
             * unless it's a nonvalidating parser and the test contains
             * external entities that the parser doesn't read.'
             *
             * We also let this apply to "valid", "invalid" and "error" tests, although
             * I'm not fully sure this is correct. */
            const QString ents(m_atts.top().value(QString(), QLatin1String("ENTITIES")).toString());
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
                failures.append(qMakePair(id, QLatin1String("Failed to open input file ") + inputFilePath));
                return true;
            }

            if(type == QLatin1String("not-wf"))
            {
                if(isWellformed(&inputFile, ParseSinglePass))
                {
                     failures.append(qMakePair(id, QLatin1String("Failed to flag ") + inputFilePath
                                                   + QLatin1String(" as not well-formed.")));

                     /* Exit, the incremental test will fail as well, no need to flood the output. */
                     return true;
                }
                else
                    successes.append(id);

                if(isWellformed(&inputFile, ParseIncrementally))
                {
                     failures.append(qMakePair(id, QLatin1String("Failed to flag ") + inputFilePath
                                                   + QLatin1String(" as not well-formed with incremental parsing.")));
                }
                else
                    successes.append(id);

                return true;
            }

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
                        failures.append(qMakePair(id, QLatin1String("Failed to open baseline ") + expectedFilePath));
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
        } else if (localName == QLatin1String("TESTCASES")
                   && m_atts.top().hasAttribute(QLatin1String("xml:base")))
            m_baseURI.pop();

        m_atts.pop();

        return true;
    }

    enum ParseMode
    {
        ParseIncrementally,
        ParseSinglePass
    };

    static bool isWellformed(QFile *const inputFile, const ParseMode mode)
    {
        if (!inputFile)
            qFatal("%s: inputFile must be a valid QFile pointer", Q_FUNC_INFO);
        if (!inputFile->isOpen())
            qFatal("%s: inputFile must be opened by the caller", Q_FUNC_INFO);
        if (mode != ParseIncrementally && mode != ParseSinglePass)
            qFatal("%s: mode must be either ParseIncrementally or ParseSinglePass", Q_FUNC_INFO);
        if (!inputFile->seek(0))
            qFatal("%s: could not seek to the beginning of the file", Q_FUNC_INFO);

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
                    reader.addData(QByteArray(buffer.data() + bufferPos, 1));
                    ++bufferPos;
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
    QStack<QXmlStreamAttributes> m_atts;
    QStack<QUrl> m_baseURI;
};
QT_BEGIN_NAMESPACE
Q_DECLARE_SHARED(TestSuiteHandler::MissedBaseline)
QT_END_NAMESPACE

class tst_QXmlStream: public QObject
{
    Q_OBJECT
public:
    tst_QXmlStream() : m_handler(QUrl::fromLocalFile(m_tempDir.filePath(catalogFile)))
    {
    }

private slots:
    void initTestCase();
    void cleanupTestCase();
    void compareCompiles();
    void runTestSuite();
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
    void writerAutoFormattingProcessingInstructionFirst() const;
    void writerAutoFormattingStartElementFirst() const;
    void writerAutoFormattingCommentFirst() const;
    void writerAutoFormattingNamespaceFirst() const;
    void addExtraNamespaceDeclarations();
    void setEntityResolver();
    void readFromQBuffer() const;
    void readFromQBufferInvalid() const;
    void readFromLatin1String() const;
    void readLatin1Document() const;
    void appendToRawDocumentWithNonUtf8Encoding_data();
    void appendToRawDocumentWithNonUtf8Encoding();
    void appendDifferentEncodingsWithoutXmlProlog_data();
    void appendDifferentEncodingsWithoutXmlProlog();
    void readNextStartElement() const;
    void readElementText() const;
    void readElementText_data() const;
    void readRawInnerData() const;
    void readRawInnerData_data() const;
    void crashInUTF16Codec() const;
    void hasAttributeSignature() const;
    void hasAttribute() const;
    void writeWithUtf8Codec() const;
    void writeWithStandalone() const;
    void writeCharacters_data() const;
    void writeCharacters() const;
    void writeAttribute_data() const;
    void writeAttribute() const;
    void writeBadCharactersUtf8_data() const;
    void writeBadCharactersUtf8() const;
    void writeBadCharactersUtf16_data() const;
    void writeBadCharactersUtf16() const;
    void entitiesAndWhitespace_1() const;
    void entitiesAndWhitespace_2() const;
    void testFalsePrematureError() const;
    void garbageInXMLPrologDefaultCodec() const;
    void garbageInXMLPrologUTF8Explicitly() const;
    void clear() const;
    void checkCommentIndentation() const;
    void checkCommentIndentation_data() const;
    void crashInXmlStreamReader() const;
    void invalidStringCharacters_data() const;
    void invalidStringCharacters() const;
    void writerErrors() const;
    void stopWritingOnError() const;
    void readBack_data() const;
    void readBack() const;
    void roundTrip() const;
    void roundTrip_data() const;
    void test_fastScanName_data() const;
    void test_fastScanName() const;

    void entityExpansionLimit() const;

    void tokenErrorHandling_data() const;
    void tokenErrorHandling() const;
    void checkStreamNotationDeclarations() const;
    void checkStreamEntityDeclarations() const;

private:
    static QByteArray readFile(const QString &filename);

    QTemporaryDir m_tempDir;
    TestSuiteHandler m_handler;
};

void tst_QXmlStream::initTestCase()
{
    // Due to license restrictions, we need to distribute part of the test
    // suit as a zip archive. So we need to unzip it before running the tests,
    // and also update some files there.
    // We also need to remove the unzipped data during cleanup.

    // On Android, we cannot unzip at the resource location, so we copy
    // everything to a temporary directory first.
    const QString XML_Test_Suite_dir = QFINDTESTDATA(xmlTestsuiteDir);
    const QString XML_Test_Suite_destDir = m_tempDir.filePath(xmlTestsuiteDir);
    copyDir(XML_Test_Suite_dir, XML_Test_Suite_destDir);


    const QString filesDir(m_tempDir.filePath(xmlconfDir));
    const QString fileName = filesDir + xmlDatasetName + ".zip";
    QVERIFY(QFile::exists(fileName));
    QZipReader reader(fileName);
    QVERIFY(reader.isReadable());
    QVERIFY(reader.extractAll(filesDir));
    // update files
    const auto files =
            QDir(filesDir + updateFilesDir).entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const auto &fileInfo : files) {
        const QString destinationPath =
                filesDir + xmlDatasetName + destinationFolder + fileInfo.fileName();
        QFile::remove(destinationPath); // copy will fail if file exists
        QVERIFY(QFile::copy(fileInfo.filePath(), destinationPath));
    }
}

void tst_QXmlStream::cleanupTestCase()
{
}

void tst_QXmlStream::compareCompiles()
{
    QTestPrivate::testEqualityOperatorsCompile<QXmlStreamAttribute>();
    QTestPrivate::testEqualityOperatorsCompile<QXmlStreamNamespaceDeclaration>();
    QTestPrivate::testEqualityOperatorsCompile<QXmlStreamNotationDeclaration>();
    QTestPrivate::testEqualityOperatorsCompile<QXmlStreamEntityDeclaration>();
}

void tst_QXmlStream::runTestSuite()
{
    QFile file(m_tempDir.filePath(catalogFile));
    QVERIFY2(file.open(QIODevice::ReadOnly),
             qPrintable(QString::fromLatin1("Failed to open the test suite catalog; %1").arg(file.fileName())));

    QVERIFY(m_handler.runTests(&file));
}

void tst_QXmlStream::reportFailures() const
{
    QFETCH(bool, isError);
    QFETCH(QString, description);

    QVERIFY2(!isError, qPrintable(description));
}

void tst_QXmlStream::reportFailures_data()
{
    const int len = m_handler.failures.size();

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

    const int len = m_handler.missedBaselines.size();

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

    const int len = m_handler.successes.size();

    for (int i = 0; i < len; ++i) {
        const QByteArray testName = QByteArray::number(i) + ". " + m_handler.successes.at(i).toLatin1();
        QTest::newRow(testName.constData()) << false;
    }

    if(len == 0)
        QTest::newRow("No test cases succeeded.") << true;
}

QByteArray tst_QXmlStream::readFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        qFatal("Could not open file %s", qPrintable(filename));

    QXmlStreamReader reader;

    reader.setDevice(&file);
    QByteArray outarray;
    QTextStream writer(&outarray);
    // We always want UTF-8, and not what the system picks up.
    writer.setEncoding(QStringConverter::Utf8);

    while (!reader.atEnd()) {
        reader.readNext();
        writer << reader.tokenString() << '(';
        if (reader.isWhitespace())
            writer << " whitespace";
        if (reader.isCDATA())
            writer << " CDATA";
        if (reader.isStartDocument() && reader.isStandaloneDocument())
            writer << " standalone";
        if (!reader.text().isEmpty())
            writer << " text=\"" << reader.text().toString() << '"';
        if (!reader.processingInstructionTarget().isEmpty())
            writer << " processingInstructionTarget=\"" << reader.processingInstructionTarget().toString() << '"';
        if (!reader.processingInstructionData().isEmpty())
            writer << " processingInstructionData=\"" << reader.processingInstructionData().toString() << '"';
        if (!reader.dtdName().isEmpty())
            writer << " dtdName=\"" << reader.dtdName().toString() << '"';
        if (!reader.dtdPublicId().isEmpty())
            writer << " dtdPublicId=\"" << reader.dtdPublicId().toString() << '"';
        if (!reader.dtdSystemId().isEmpty())
            writer << " dtdSystemId=\"" << reader.dtdSystemId().toString() << '"';
        if (!reader.documentVersion().isEmpty())
            writer << " documentVersion=\"" << reader.documentVersion().toString() << '"';
        if (!reader.documentEncoding().isEmpty())
            writer << " documentEncoding=\"" << reader.documentEncoding().toString() << '"';
        if (!reader.name().isEmpty())
            writer << " name=\"" << reader.name().toString() << '"';
        if (!reader.namespaceUri().isEmpty())
            writer << " namespaceUri=\"" << reader.namespaceUri().toString() << '"';
        if (!reader.qualifiedName().isEmpty())
            writer << " qualifiedName=\"" << reader.qualifiedName().toString() << '"';
        if (!reader.prefix().isEmpty())
            writer << " prefix=\"" << reader.prefix().toString() << '"';
        const auto attributes = reader.attributes();
        if (attributes.size()) {
            for (const QXmlStreamAttribute &attribute : attributes) {
                writer << Qt::endl << "    Attribute(";
                if (!attribute.name().isEmpty())
                    writer << " name=\"" << attribute.name().toString() << '"';
                if (!attribute.namespaceUri().isEmpty())
                    writer << " namespaceUri=\"" << attribute.namespaceUri().toString() << '"';
                if (!attribute.qualifiedName().isEmpty())
                    writer << " qualifiedName=\"" << attribute.qualifiedName().toString() << '"';
                if (!attribute.prefix().isEmpty())
                    writer << " prefix=\"" << attribute.prefix().toString() << '"';
                if (!attribute.value().isEmpty())
                    writer << " value=\"" << attribute.value().toString() << '"';
                writer << " )" << Qt::endl;
            }
        }
        const auto namespaceDeclarations = reader.namespaceDeclarations();
        if (namespaceDeclarations.size()) {
            for (const QXmlStreamNamespaceDeclaration &namespaceDeclaration : namespaceDeclarations) {
                writer << Qt::endl << "    NamespaceDeclaration(";
                if (!namespaceDeclaration.prefix().isEmpty())
                    writer << " prefix=\"" << namespaceDeclaration.prefix().toString() << '"';
                if (!namespaceDeclaration.namespaceUri().isEmpty())
                    writer << " namespaceUri=\"" << namespaceDeclaration.namespaceUri().toString() << '"';
                writer << " )" << Qt::endl;
            }
        }
        const auto notationDeclarations = reader.notationDeclarations();
        if (notationDeclarations.size()) {
            for (const QXmlStreamNotationDeclaration &notationDeclaration : notationDeclarations) {
                writer << Qt::endl << "    NotationDeclaration(";
                if (!notationDeclaration.name().isEmpty())
                    writer << " name=\"" << notationDeclaration.name().toString() << '"';
                if (!notationDeclaration.systemId().isEmpty())
                    writer << " systemId=\"" << notationDeclaration.systemId().toString() << '"';
                if (!notationDeclaration.publicId().isEmpty())
                    writer << " publicId=\"" << notationDeclaration.publicId().toString() << '"';
                writer << " )" << Qt::endl;
            }
        }
        const auto entityDeclarations = reader.entityDeclarations();
        if (entityDeclarations.size()) {
            for (const QXmlStreamEntityDeclaration &entityDeclaration : entityDeclarations) {
                writer << Qt::endl << "    EntityDeclaration(";
                if (!entityDeclaration.name().isEmpty())
                    writer << " name=\"" << entityDeclaration.name().toString() << '"';
                if (!entityDeclaration.notationName().isEmpty())
                    writer << " notationName=\"" << entityDeclaration.notationName().toString() << '"';
                if (!entityDeclaration.systemId().isEmpty())
                    writer << " systemId=\"" << entityDeclaration.systemId().toString() << '"';
                if (!entityDeclaration.publicId().isEmpty())
                    writer << " publicId=\"" << entityDeclaration.publicId().toString() << '"';
                if (!entityDeclaration.value().isEmpty())
                    writer << " value=\"" << entityDeclaration.value().toString() << '"';
                writer << " )" << Qt::endl;
            }
        }
        writer << " )" << Qt::endl;
    }
    if (reader.hasError())
        writer << "ERROR: " << reader.errorString() << Qt::endl;
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
    const auto fileNames = dir.entryList(QStringList() << "*.xml");
    for (const QString &filename : fileNames) {
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
        QXmlStreamNamespaceDeclaration undeclared("undeclared", "blabla");
        QXmlStreamNamespaceDeclaration undeclared_too("undeclared_too", "blabla");
        xml.addExtraNamespaceDeclaration(undeclared);
        xml.addExtraNamespaceDeclaration(undeclared_too);
        while (!xml.atEnd()) {
            xml.readNext();
        }
        QVERIFY2(!xml.hasError(), xml.errorString().toLatin1().constData());
        QT_TEST_EQUALITY_OPS(undeclared, undeclared_too, false);
        undeclared = undeclared_too;
        QT_TEST_EQUALITY_OPS(undeclared, undeclared_too, true);
    }
}


class EntityResolver : public QXmlStreamEntityResolver {
public:
    QString resolveUndeclaredEntity(const QString &name) override {
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
        QCOMPARE(xml.error(), QXmlStreamReader::PrematureEndOfDocumentError);
        QCOMPARE(xml.errorString(), QLatin1String("Premature end of document."));
        xml.addData(legal_start);
        while (!xml.atEnd()) {
            xml.readNext();
        }
        QCOMPARE(xml.error(), QXmlStreamReader::PrematureEndOfDocumentError);
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
        QCOMPARE(xml.error(), QXmlStreamReader::NotWellFormedError);
    }
}

// Regression test for crash due to using empty QStack.
void tst_QXmlStream::writerHangs() const
{
    QTemporaryDir dir(QDir::tempPath() + QLatin1String("/tst_qxmlstream.XXXXXX"));
    QFile file(dir.path() + "/test.xml");

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
    QString s = QLatin1String("<?xml version=\"1.0\" encoding=\"UTF-8\"?><A attribute=\"value")
        + QChar(QChar::Nbsp) + QLatin1String("\"/>\n");
    QCOMPARE(buffer.buffer().data(), s.toUtf8().data());
}

void tst_QXmlStream::writerAutoFormattingProcessingInstructionFirst() const
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QXmlStreamWriter writer(&buffer);
    writer.setAutoFormatting(true);
    writer.writeProcessingInstruction("B");
    writer.writeComment("This is a comment");
    writer.writeStartElement("A");
    writer.writeNamespace("http://website.com", "website");
    writer.writeDefaultNamespace("http://websiteNo2.com");
    writer.writeEndElement();
    writer.writeEndDocument();
    const char *str =
            "<?B?>\n<!--This is a comment-->\n<A xmlns:website=\"http://website.com\" xmlns=\"http://websiteNo2.com\"/>\n";
    QCOMPARE(buffer.buffer().data(), str);
}
void tst_QXmlStream::writerAutoFormattingStartElementFirst() const
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QXmlStreamWriter writer(&buffer);
    writer.setAutoFormatting(true);
    writer.writeStartElement("A");
    writer.writeNamespace("http://website.com", "website");
    writer.writeComment("This is a comment");
    writer.writeProcessingInstruction("B");
    writer.writeDefaultNamespace("http://websiteNo2.com");
    writer.writeEndElement();
    writer.writeEndDocument();
    const char *str = "<A xmlns:website=\"http://website.com\">\n    <!--This is a comment-->\n    <?B?>\n</A>\n";
    QCOMPARE(buffer.buffer().data(), str);
}

void tst_QXmlStream::writerAutoFormattingCommentFirst() const
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QXmlStreamWriter writer(&buffer);
    writer.setAutoFormatting(true);
    writer.writeComment("This is a comment");
    writer.writeStartElement("A");
    writer.writeNamespace("http://website.com", "website");
    writer.writeEndElement();
    writer.writeDefaultNamespace("http://websiteNo2.com");
    writer.writeProcessingInstruction("B");
    writer.writeEndDocument();
    const char *str = "<!--This is a comment--><A xmlns:website=\"http://website.com\"/>\n<?B?>\n";
    QCOMPARE(buffer.buffer().data(), str);
}

void tst_QXmlStream::writerAutoFormattingNamespaceFirst() const
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QXmlStreamWriter writer(&buffer);
    writer.setAutoFormatting(true);
    writer.writeNamespace("http://website.com", "website");
    writer.writeStartElement("A");
    writer.writeDefaultNamespace("http://websiteNo2.com");
    writer.writeProcessingInstruction("B");
    writer.writeEndElement();
    writer.writeComment("This is a comment");
    writer.writeEndDocument();
    const char *str = "<A xmlns:website=\"http://website.com\" xmlns=\"http://websiteNo2.com\">\n    <?B?></A>\n<!--This is a comment-->\n";
    QCOMPARE(buffer.buffer().data(), str);
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

void tst_QXmlStream::readFromLatin1String() const
{
    const auto in = "<a>M\xE5rten</a>"_L1;
    {
        QXmlStreamReader reader(in);
        QVERIFY(reader.readNextStartElement());
        QString text = reader.readElementText();
        QCOMPARE(text, "M\xE5rten"_L1);
    }
    // Same as above, but with addData()
    {
        QXmlStreamReader reader;
        reader.addData(in);
        QVERIFY(reader.readNextStartElement());
        QString text = reader.readElementText();
        QCOMPARE(text, "M\xE5rten"_L1);
    }
}

void tst_QXmlStream::readLatin1Document() const
{
    const auto in = "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?><a>M\xE5rten</a>"_L1;
    {
        QXmlStreamReader reader(in);
        QVERIFY(reader.readNextStartElement());
        QString text = reader.readElementText();
        QCOMPARE(text, "M\xE5rten"_L1);
    }
    // Same as above, but with addData(), QTBUG-135033
    {
        QXmlStreamReader reader;
        reader.addData(in);
        QVERIFY(reader.readNextStartElement());
        QString text = reader.readElementText();
        QCOMPARE(text, "M\xE5rten"_L1);
    }
}

void tst_QXmlStream::appendToRawDocumentWithNonUtf8Encoding_data()
{
    QTest::addColumn<QByteArray>("rawDocumentStart");
    QTest::addColumn<QString>("expectedFirstElementText");
    QTest::addColumn<QString>("nextData");
    QTest::addColumn<QStringConverter::Encoding>("nextEncoding");
    QTest::addColumn<QString>("expectedNextElementText");

    auto row = [](const char *name, const QByteArray &encoding,
                  const QByteArray &firstData, const QString &expectedFirstString,
                  QStringConverter::Encoding nextEncoding, const QString &nextString) {
        const QByteArray docStart = "<?xml version=\"1.0\" encoding=\"" + encoding
                + "\"?><foo><a>" + firstData + "</a>";
        const QString nextElement = u"<a>"_s + nextString + u"</a>"_s;
        QTest::newRow(name) << docStart << expectedFirstString << nextElement
                            << nextEncoding << nextString;
    };

    row("l1+utf16", "iso-8859-1"_ba, "M\xE5rten"_ba, QString::fromLatin1("M\xE5rten"),
        QStringConverter::Utf16, u"M\u00E5rten"_s);
    row("l1+utf8", "iso-8859-1"_ba, "M\xE5rten"_ba, QString::fromLatin1("M\xE5rten"),
        QStringConverter::Utf8, QString::fromUtf8("M\xC3\xA5rten"));
    row("l1+l1", "iso-8859-1"_ba, "M\xE5rten"_ba, QString::fromLatin1("M\xE5rten"),
        QStringConverter::Latin1, QString::fromLatin1("M\xE5rten"));

    const QString utf16Str = u"<?xml version=\"1.0\" encoding=\"utf-16\"?>"
                             "<foo><a>M\u00E5rten</a>"_s;
    const QByteArray utf16Data{reinterpret_cast<const char *>(utf16Str.utf16()),
                               utf16Str.size() * 2};

    QTest::newRow("utf16+utf16") << utf16Data << u"M\u00E5rten"_s
                                 << u"<a>M\u00E5rten</a>"_s
                                 << QStringConverter::Utf16
                                 << u"M\u00E5rten"_s;
}

void tst_QXmlStream::appendToRawDocumentWithNonUtf8Encoding()
{
    QFETCH(const QByteArray, rawDocumentStart);
    QFETCH(const QString, expectedFirstElementText);
    QFETCH(const QString, nextData);
    QFETCH(const QStringConverter::Encoding, nextEncoding);
    QFETCH(const QString, expectedNextElementText);

    QXmlStreamReader reader(rawDocumentStart);
    QVERIFY(reader.readNextStartElement()); // foo
    QVERIFY(reader.readNextStartElement()); // a
    QString text = reader.readElementText();
    QCOMPARE(text, expectedFirstElementText);

    switch (nextEncoding) {
    case QStringConverter::Utf16:
        reader.addData(nextData);
        break;
    case QStringConverter::Utf8:
        reader.addData(QUtf8StringView{nextData.toUtf8()});
        break;
    case QStringConverter::Latin1:
        reader.addData(QLatin1StringView{nextData.toLatin1()});
        break;
    default:
        Q_UNREACHABLE();
    }
    QVERIFY(reader.readNextStartElement()); // a
    text = reader.readElementText();

    QCOMPARE(text, expectedNextElementText);
}

struct DataAndEncoding
{
    enum Encoding : quint8 {
        Raw = 0,
        Latin1,
        Utf8,
        Utf16
    };

    QByteArray data;
    Encoding encoding;

    DataAndEncoding(const QByteArray &d, Encoding e)
        : data(d), encoding(e)
    {}
    DataAndEncoding(const QString &str)
        : data(asUtf16ByteArray(str)), encoding(Encoding::Utf16)
    {}

    static QByteArray asUtf16ByteArray(const QString &input)
    {
        return QByteArray{reinterpret_cast<const char *>(input.utf16()), input.size() * 2};
    }

    QAnyStringView toAnyStringView() const
    {
        switch (encoding) {
        case Latin1:
            return QLatin1StringView{data};
        case Utf8:
            return QUtf8StringView{data};
        case Utf16:
            Q_ASSERT(data.size() % 2 == 0);
            return QStringView{reinterpret_cast<const char16_t *>(data.data()), data.size() / 2};
        case Raw:
            // Impossible to convert to QASV in general case
            break;
        }
        Q_UNREACHABLE_RETURN({});
    }

    // for next_permutation
    friend bool operator<(const DataAndEncoding &lhs, const DataAndEncoding &rhs)
    {
        return lhs.encoding < rhs.encoding;
    }
};

void tst_QXmlStream::appendDifferentEncodingsWithoutXmlProlog_data()
{
    QTest::addColumn<QList<DataAndEncoding>>("inputs");
    QTest::addColumn<QString>("expectedResult");

    const QByteArray u8Str = "ΔΩΘ";
    const QByteArray l1Str = "\xC4\xD6\xDC"; // ÄÖÜ
    const QByteArray rawDataUtf8 = "\xf0\x9f\x98\x82"; // FACE WITH TEARS OF JOY (U+1F602)
    const QString u16Str = u"\U0001F60E"_s; // SMILING FACE WITH SUNGLASSES (U+1F60E)

    using Enc = DataAndEncoding::Encoding;

    QVarLengthArray<DataAndEncoding> inputs{ DataAndEncoding{u8Str, Enc::Utf8},
                                             DataAndEncoding{l1Str, Enc::Latin1},
                                             DataAndEncoding{rawDataUtf8, Enc::Raw},
                                             DataAndEncoding{u16Str} };

    // Helper function to populate test data
    auto encToName = [](Enc e) -> QByteArray {
        switch (e) {
        case Enc::Raw:
            return "bytes"_ba;
        case Enc::Latin1:
            return "l1"_ba;
        case Enc::Utf8:
            return "u8"_ba;
        case Enc::Utf16:
            return "u16"_ba;
        }
        Q_UNREACHABLE_RETURN("");
    };
    auto adjustFirst = [](const DataAndEncoding &input) -> DataAndEncoding {
        QByteArray newData = input.data;
        if (input.encoding == Enc::Utf16)
            newData.prepend(DataAndEncoding::asUtf16ByteArray(u"<a>"_s));
        else
            newData.prepend("<a>"_ba);
        return {newData, input.encoding};
    };
    auto adjustLast = [](const DataAndEncoding &input) -> DataAndEncoding {
        QByteArray newData = input.data;
        if (input.encoding == Enc::Utf16)
            newData.append(DataAndEncoding::asUtf16ByteArray(u"</a>"_s));
        else
            newData.append("</a>"_ba);
        return {newData, input.encoding};
    };
    auto dataToString = [](const DataAndEncoding &input) -> QString {
        if (input.encoding == Enc::Raw) {
            // This function treats raw data as UTF-8
            return QString::fromUtf8(input.data);
        }
        return input.toAnyStringView().toString();
    };
    // Iterate over all permutations of the list.
    // Sort the list first, to cover all cases
    std::sort(inputs.begin(), inputs.end());
    do {
        const auto lastIdx = inputs.size() - 1;
        QByteArray testName;
        QList<DataAndEncoding> inputData;
        QString expectedResult;
        for (qsizetype i = 0; i <= lastIdx; ++i) {
            const auto &item = inputs[i];
            testName += encToName(item.encoding);
            if (i != lastIdx)
                testName.append('+');
            if (i == 0)
                inputData.append(adjustFirst(item));
            else if (i == lastIdx)
                inputData.append(adjustLast(item));
            else
                inputData.append(item);
            expectedResult.append(dataToString(item));
        }
        QTest::newRow(testName.constData()) << inputData << expectedResult;
    } while (std::next_permutation(inputs.begin(), inputs.end()));

    // plus add some corner cases

    QTest::newRow("u8+bytes_FACE_WITH_TEARS_OF_JOY")
            << QList{ DataAndEncoding{"<a>\xf0\x9f"_ba, Enc::Utf8},
                      DataAndEncoding{"\x98\x82</a>"_ba, Enc::Raw} }
            << u"\U0001F602"_s;

    // The test tries to read FACE IN CLOUDS emoji.
    // Its full representation is:
    // - FACE WITHOUT MOUTH: U+1F636 or \xf0\x9f\x98\xb6;
    // - ZERO WIDTH JOINER: U+200D or \xe2\x80\x8d;
    // - FOG: U+1F32B or \xf0\x9f\x8c\xab;
    // - VARIATION SELECTOR-16: U+FE0F or \xef\xb8\x8f.
    // This test tries to encode a part of it as UTF-8, and the rest as UTF-16.
    // Important is that we need to break at the borders of the characters
    QTest::newRow("u8+u16_FACE_IN_CLOUDS")
            << QList{ DataAndEncoding{"<a>\xf0\x9f\x98\xb6\xe2\x80\x8d"_ba, Enc::Utf8},
                      DataAndEncoding{u"\U0001F32B\uFE0F</a>"_s} }
            << u"\U0001F636\u200D\U0001F32B\uFE0F"_s;
}

void tst_QXmlStream::appendDifferentEncodingsWithoutXmlProlog()
{
    QFETCH(const QList<DataAndEncoding>, inputs);
    QFETCH(const QString, expectedResult);

    {
        QXmlStreamReader reader;
        for (const auto &data : inputs) {
            if (data.encoding == DataAndEncoding::Raw)
                reader.addData(data.data);
            else
                reader.addData(data.toAnyStringView());
        }
        QVERIFY(reader.readNextStartElement());
        const QString text = reader.readElementText();
        QCOMPARE(text, expectedResult);
    }
    // same with c-tor
    {
        std::unique_ptr<QXmlStreamReader> reader = nullptr;
        for (const auto &data : inputs) {
            if (!reader) {
                if (data.encoding == DataAndEncoding::Raw)
                    reader = std::make_unique<QXmlStreamReader>(data.data);
                else
                    reader = std::make_unique<QXmlStreamReader>(data.toAnyStringView());
            } else {
                if (data.encoding == DataAndEncoding::Raw)
                    reader->addData(data.data);
                else
                    reader->addData(data.toAnyStringView());
            }
        }
        QVERIFY(reader->readNextStartElement());
        const QString text = reader->readElementText();
        QCOMPARE(text, expectedResult);
    }
}

void tst_QXmlStream::readNextStartElement() const
{
    QLatin1String in("<?xml version=\"1.0\"?><A><!-- blah --><B><C/></B><B attr=\"value\"/>text</A>");
    QXmlStreamReader reader(in);

    QVERIFY(reader.readNextStartElement());
    QVERIFY(reader.isStartElement() && reader.name() == QLatin1String("A"));

    int amountOfB = 0;
    while (reader.readNextStartElement()) {
        QVERIFY(reader.isStartElement() && reader.name() == QLatin1String("B"));
        ++amountOfB;
        reader.skipCurrentElement();
    }

    QCOMPARE(amountOfB, 2);

    // well-formed document end follows
    QVERIFY(!reader.readNextStartElement());
    QCOMPARE(reader.error(), QXmlStreamReader::NoError);
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

void tst_QXmlStream::readRawInnerData() const
{
    QFETCH(QString, input);
    QFETCH(QString, expected);
    QFETCH(QXmlStreamReader::Error, expectedError);
    QXmlStreamReader reader(input);

    reader.readNextStartElement();
    QCOMPARE(reader.readRawInnerData(), expected);

    if (reader.hasError())
        QCOMPARE(reader.error(), expectedError);
}

void tst_QXmlStream::readRawInnerData_data() const
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<QXmlStreamReader::Error>("expectedError");
    // Valid cases
    const QString mixedTags =
            u"<root>\n"
            "    <!-- A comment -->\n"
            "    Some text\n"
            "    <b attr=\"val\">bold <![CDATA[stuff]]></b>\n"
            "    <?pi info?>\n"
            "</root>"_s;
    const QString mixedTagsResult =
            u"\n    <!-- A comment -->\n"
            "    Some text\n"
            "    <b attr=\"val\">bold <![CDATA[stuff]]></b>\n"
            "    <?pi info?>\n"_s;
    const QString nestedTokensEntities =
            u"<root>\n"
            "    <firstChild attr=\"&quot;attrValue&quot;\">\n"
            "        <secondChild attr=\"&lt;child&gt;\">Some &amp; text\n"
            "        </secondChild>\n"
            "    </firstChild>\n"
            "</root>"_s;
    const QString nestedTokensEntitiesResult =
            u"\n    <firstChild attr=\"&quot;attrValue&quot;\">"
            "\n        <secondChild attr=\"&lt;child&gt;\">"
            "Some &amp; text\n"
            "        </secondChild>\n"
            "    </firstChild>\n"_s;
    const QString emptyNested =
            u"<root>\n"
            "   <!-- A comment -->\n"
            "   <empty />\n"
            "</root>"_s;
    const QString emptyNestedResult =
            u"\n   <!-- A comment -->\n"
            "   <empty></empty>\n"_s;

    const QString plainText = u"<root>Just some text</root>"_s;
    const QString expectedPlainText = "Just some text";

    const QString cdataInput =
            u"<root>"
            "<![CDATA[Some <cdata> content]]>"
            "</root>"_s;
    const QString expectedCdata = u"<![CDATA[Some <cdata> content]]>"_s;

    const QString commentInput = u"<root><!-- A comment --></root>"_s;
    const QString expectedComment = u"<!-- A comment -->"_s;

    const QString PIInput =u"<root>\n    <?pi data?>\n</root>"_s;
    const QString expectedPI =u"\n    <?pi data?>\n"_s;

    const QString nestedInput = u"<root>\n"
                          "    <outer>\n    <inner>text</inner>\n"
                          "    </outer>\n</root>"_s;
    const QString expectedNested = u"\n    <outer>\n"
                             "    <inner>text</inner>\n"
                             "    </outer>\n"_s;
    const QString customEntity =
            u"<!DOCTYPE root [<!ENTITY ent \"Resolved entity\">]>\n"
            "<root>\n"
            "    <child>Some text and &ent;</child>\n"
            "</root>"_s;
    const QString customEntityResult =
            u"\n    <child>Some text and Resolved entity</child>\n"_s;

    //Invalid cases
    const QString noTokensText = u"Just text without tokens"_s;
    const QString mismatchedTags = u"<root>\n    <child></differentChild>\n</root>"_s;
    const QString mismatchedTagsResult = u"\n    <child>"_s;
    const QString unclosedComment = u"<root>\n    <!-- Comment starts\n"
                              "    <child>Text</child>\n"
                              "</root>"_s;
    const QString nestedComment =
            u"<root>\n"
            "    <!-- A comment <!-- Nested comment --> -->\n"
            "</root>"_s;
    const QString unclosedCDATA = u" <root>\n    <![CDATA[ Unclosed CDATA\n"
                            "    <child>Text</child>\n"
                            "</root>"_s;
    const QString missingClosingTag = u"<root>\n    <child>Text\n</root>"_s;
    const QString missingClosingTagResult = u"\n    <child>Text\n"_s;
    const QString noValueAttr = u"<root>\n    <child attr=>text</child>\n</root>"_s;
    const QString invalidAttrName =
            u"<root>\n"
            "    <child 123attr=\"value\"></child>\n"
            "</root>"_s;
    const QString invalidChars = u"<root>\n    <ch@ld></ch@ld>\n</root>"_s;
    const QString tooManyElements = u"<first></first><second></second>"_s;
    const QString misplacedDecl =
            u"<root>\n    <?xml version=\"1.0\"?>\n"
            "<child>Text</child></root>"_s;

    const QString emptyResult = u"\n    "_s;

    QTest::newRow("allTokens")            << mixedTags             << mixedTagsResult
                                          << QXmlStreamReader::NoError;
    QTest::newRow("plainTextOnly")        << plainText             << expectedPlainText
                                          << QXmlStreamReader::NoError;
    QTest::newRow("CDATAOnly")            << cdataInput            << expectedCdata
                                          << QXmlStreamReader::NoError;
    QTest::newRow("commentOnly")          << commentInput          << expectedComment
                                          << QXmlStreamReader::NoError;
    QTest::newRow("PIOnly")               << PIInput               << expectedPI
                                          << QXmlStreamReader::NoError;
    QTest::newRow("nestedElements")       << nestedInput           << expectedNested
                                          << QXmlStreamReader::NoError;
    QTest::newRow("nestedTokensEntities") << nestedTokensEntities  << nestedTokensEntitiesResult
                                          << QXmlStreamReader::NoError;
    QTest::newRow("emptyNested")          << emptyNested           << emptyNestedResult
                                          << QXmlStreamReader::NoError;
    QTest::newRow("customEntity")         << customEntity          << customEntityResult
                                          << QXmlStreamReader::NoError;

    QTest::newRow("noTokensText")       << noTokensText      << QString()
                                        << QXmlStreamReader::NotWellFormedError;
    QTest::newRow("mismatchedTags")     << mismatchedTags    << mismatchedTagsResult
                                        << QXmlStreamReader::NotWellFormedError;
    QTest::newRow("unclosedComment")    << unclosedComment   << emptyResult
                                        << QXmlStreamReader::PrematureEndOfDocumentError;
    QTest::newRow("nestedComment")      << nestedComment     << emptyResult
                                        << QXmlStreamReader::NotWellFormedError;
    QTest::newRow("unclosedCDATA")      << unclosedCDATA     << emptyResult
                                        << QXmlStreamReader::PrematureEndOfDocumentError;
    QTest::newRow("missingClosingTag")  << missingClosingTag << missingClosingTagResult
                                        << QXmlStreamReader::NotWellFormedError;
    QTest::newRow("noValueAttr")        << noValueAttr       << emptyResult
                                        << QXmlStreamReader::NotWellFormedError;
    QTest::newRow("invalidAttrName")    << invalidAttrName   << emptyResult
                                        << QXmlStreamReader::NotWellFormedError;
    QTest::newRow("invalidChars")       << invalidChars      << emptyResult
                                        << QXmlStreamReader::NotWellFormedError;
    QTest::newRow("tooManyElements")    << tooManyElements   << QString()
                                        << QXmlStreamReader::NotWellFormedError;
    QTest::newRow("misplacedDecl")      << misplacedDecl     << emptyResult
                                        << QXmlStreamReader::NotWellFormedError;
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
        myStdOut << canonical << Qt::endl;
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
    auto xml = QStringLiteral("<e"
                              "  xmlns:p='http://example.com/2'"
                              "  xmlns='http://example.com/'"
                              "  attr1='value'"
                              "  attr2='value2'"
                              "  p:attr3='value3'"
                              "  emptyAttr=''"
                              "  atträbute='meep'"
                              "  α='β'"
                              "  >"
                              "    <noAttributes/>"
                              "</e>");

    QXmlStreamReader reader(xml);

    QCOMPARE(reader.readNext(), QXmlStreamReader::StartDocument);
    QCOMPARE(reader.readNext(), QXmlStreamReader::StartElement);
    const QXmlStreamAttributes &atts = reader.attributes();

    /* QLatin1String overload. */
    QVERIFY(atts.hasAttribute(QLatin1String("attr1")));
    QVERIFY(atts.hasAttribute(QLatin1String("attr2")));
    QVERIFY(atts.hasAttribute(QLatin1String("p:attr3")));
    QVERIFY(atts.hasAttribute(QLatin1String("emptyAttr")));
    QVERIFY(atts.hasAttribute(QLatin1String("attr\xE4""bute")));
    // α is not representable in L1...
    QVERIFY(!atts.hasAttribute(QLatin1String("DOESNOTEXIST")));

    /* string literals (UTF-8/16) */
    QVERIFY(atts.hasAttribute(u8"atträbute"));
    QVERIFY(atts.hasAttribute( u"atträbute"));
    QVERIFY(atts.hasAttribute(u8"α"));
    QVERIFY(atts.hasAttribute( u"α"));
    QVERIFY(!atts.hasAttribute(u8"β"));
    QVERIFY(!atts.hasAttribute( u"β"));

    /* Test with an empty & null namespaces. */
    QVERIFY(atts.hasAttribute(QString(), QLatin1String("attr2"))); /* A null string. */
    QVERIFY(atts.hasAttribute(QLatin1String(""), QLatin1String("attr2"))); /* An empty string. */

    /* QString overload. */
    QVERIFY(atts.hasAttribute(QString::fromLatin1("attr1")));
    QVERIFY(atts.hasAttribute(QString::fromLatin1("attr2")));
    QVERIFY(atts.hasAttribute(QString::fromLatin1("p:attr3")));
    QVERIFY(atts.hasAttribute(QStringLiteral("atträbute")));
    QVERIFY(atts.hasAttribute(QStringLiteral("α")));
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
    QCOMPARE(reader.readNext(), QXmlStreamReader::Characters);
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

    QXmlStreamAttribute attrValue1(QLatin1String("http://example.com/"), QString::fromLatin1("attr1"));
    QXmlStreamAttribute attrValue2 = atts.at(0);
    QT_TEST_EQUALITY_OPS(atts.at(0), QXmlStreamAttribute(), false);
    QT_TEST_EQUALITY_OPS(atts.at(0), attrValue1, false);
    QT_TEST_EQUALITY_OPS(atts.at(0), attrValue2, true);
    QT_TEST_EQUALITY_OPS(attrValue1, attrValue2, false);
    attrValue1 = attrValue2;
    QT_TEST_EQUALITY_OPS(attrValue1, attrValue2, true);
}

void tst_QXmlStream::writeWithUtf8Codec() const
{
    QByteArray outarray;
    QXmlStreamWriter writer(&outarray);

    writer.writeStartDocument("1.0");
    static const char begin[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    QVERIFY(outarray.startsWith(begin));
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

static void writeCharacters_data_common()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("output");

    QTest::newRow("empty") << QString() << QString();

    // invalid content
    QTest::newRow("null-character") << u"\0"_s << QString();
    QTest::newRow("vertical-tab") << "\v" << QString();
    QTest::newRow("form-feed") << "\f" << QString();
    QTest::newRow("esc") << "\x1f" << QString();
    QTest::newRow("U+FFFE") << u"\xfffe"_s << QString();
    QTest::newRow("U+FFFF") << u"\xffff"_s << QString();

    // simple strings
    QTest::newRow("us-ascii") << "Hello, world" << "Hello, world";
    QTest::newRow("latin1") << "Bokmål" << "Bokmål";
    QTest::newRow("nonlatin1") << "Ελληνικά" << "Ελληνικά";
    QTest::newRow("nonbmp") << u"\U00010000"_s << u"\U00010000"_s;

    // escaped content
    QTest::newRow("less-than") << "<" << "&lt;";
    QTest::newRow("greater-than") << ">" << "&gt;";
    QTest::newRow("ampersand") << "&" << "&amp;";
    QTest::newRow("quote") << "\"" << "&quot;";
}

template <typename Execute, typename Transform>
static void writeCharacters_common(Execute &&exec, Transform &&transform)
{
    QFETCH(QString, input);
    QFETCH(QString, output);
    QStringView utf16 = input;
    QByteArray utf8ba = input.toUtf8();
    QUtf8StringView utf8(utf8ba);

    // may be invalid if input is not Latin1
    QByteArray l1ba = input.toLatin1();
    QLatin1StringView l1(l1ba);
    if (l1 != input)
        l1 = {};

    auto write = [&](auto input) -> std::optional<QString> {
        QString result;
        QXmlStreamWriter writer(&result);
        writer.writeStartElement("a");
        exec(writer, input);
        writer.writeEndElement();
        if (writer.hasError())
            return std::nullopt;
        return result;
    };

    if (input.isNull() != output.isNull()) {
        // error
        QCOMPARE(write(utf16), std::nullopt);
        QCOMPARE(write(utf8), std::nullopt);
        if (!l1.isEmpty())
            QCOMPARE(write(l1), std::nullopt);
    } else {
        output = transform(output);
        QCOMPARE(write(utf16), output);
        QCOMPARE(write(utf8), output);
        if (!l1.isEmpty())
            QCOMPARE(write(l1), output);
    }
}

void tst_QXmlStream::writeCharacters_data() const
{
    writeCharacters_data_common();
    QTest::newRow("tab") << "\t" << "\t";
    QTest::newRow("newline") << "\n" << "\n";
    QTest::newRow("carriage-return") << "\r" << "\r";
}

void tst_QXmlStream::writeCharacters() const
{
    auto exec = [](QXmlStreamWriter &writer, auto input) {
        writer.writeCharacters(input);
    };
    auto transform = [](auto output) { return "<a>" + output + "</a>"; };
    writeCharacters_common(exec, transform);
}

void tst_QXmlStream::writeAttribute_data() const
{
    writeCharacters_data_common();
    QTest::newRow("tab") << "\t" << "&#9;";
    QTest::newRow("newline") << "\n" << "&#10;";
    QTest::newRow("carriage-return") << "\r" << "&#13;";
}

void tst_QXmlStream::writeAttribute() const
{
    auto exec = [](QXmlStreamWriter &writer, auto input) {
        writer.writeAttribute("b", input);
    };
    auto transform = [](auto output) { return "<a b=\"" + output + "\"/>"; };
    writeCharacters_common(exec, transform);
}

#include "../../io/qurlinternal/utf8data.cpp"
void tst_QXmlStream::writeBadCharactersUtf8_data() const
{
    QTest::addColumn<QByteArray>("input");
    loadInvalidUtf8Rows();
}

void tst_QXmlStream::writeBadCharactersUtf8() const
{
    QFETCH(QByteArray, input);
    QString target;
    QXmlStreamWriter writer(&target);
    writer.writeTextElement("a", QUtf8StringView(input));
    QVERIFY(writer.hasError());
    QCOMPARE(writer.error(), QXmlStreamWriter::Error::Encoding);
}

void tst_QXmlStream::writeBadCharactersUtf16_data() const
{
    QTest::addColumn<QString>("input");
    QTest::addRow("low-surrogate") << u"\xdc00"_s;
    QTest::addRow("high-surrogate") << u"\xd800"_s;
    QTest::addRow("inverted-surrogate-pair") << u"\xdc00\xd800"_s;
    QTest::addRow("high-surrogate+non-surrogate") << u"\xd800z"_s;
}

void tst_QXmlStream::writeBadCharactersUtf16() const
{
    QFETCH(QString, input);
    QString target;
    QXmlStreamWriter writer(&target);
    writer.writeTextElement("a", input);
    QVERIFY(writer.hasError());
    QCOMPARE(writer.error(), QXmlStreamWriter::Error::Encoding);

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
    qint64 writeData(const char *c, qint64 i) override
    {
        qint64 ai = qMin(m_capacity, i);
        m_capacity -= ai;
        return ai ? QBuffer::writeData(c, ai) : 0;
    }
public:
    void setCapacity(int capacity) { m_capacity = capacity; }
private:
    qint64 m_capacity = 0;
};

void tst_QXmlStream::writerErrors() const
{
    {
        FakeBuffer fb;
        QVERIFY(fb.open(QBuffer::ReadWrite));
        fb.setCapacity(1000);
        QXmlStreamWriter writer(&fb);
        writer.writeStartDocument();
        writer.writeEndDocument();
        QVERIFY(!writer.hasError());
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::None);
        QVERIFY(writer.errorString().isEmpty());
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
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::IO);
        QVERIFY(!writer.errorString().isEmpty());
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
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::IO);
        QVERIFY(!writer.errorString().isEmpty());
        QCOMPARE(fb.data(), expected);
    }

    {
        // Failure caused by write(QStringRef)
        FakeBuffer fb;
        QVERIFY(fb.open(QBuffer::ReadWrite));
        const QByteArray expected =
                QByteArrayLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?><test xmlns:");
        fb.setCapacity(expected.size());
        QXmlStreamWriter writer(&fb);
        writer.writeStartDocument();
        writer.writeStartElement("test");
        writer.writeNamespace("http://foo.bar", "foo");
        QVERIFY(writer.hasError());
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::IO);
        QVERIFY(!writer.errorString().isEmpty());
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
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::IO);
        QCOMPARE(fb.data(), QByteArray("<?xml vers"));
        fb.setCapacity(1000);
        writer.writeStartElement("test"); // literal & qstring
        writer.writeNamespace("http://foo.bar", "foo"); // literal & qstringref
        QVERIFY(writer.hasError());
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::IO);
        QVERIFY(!writer.errorString().isEmpty());
        QCOMPARE(fb.data(), QByteArray("<?xml vers"));
    }

    {
        // Encoding error: lone high surrogate
        QByteArray buffer;
        QXmlStreamWriter writer(&buffer);
        writer.writeStartDocument();
        writer.writeStartElement("root");
        writer.writeCharacters(QChar(0xD800));
        writer.writeEndElement();
        writer.writeEndDocument();
        QVERIFY(writer.hasError());
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::Encoding);
        QVERIFY(!writer.errorString().isEmpty());
    }

    {
        // Invalid character error: invalid character for XML 1.0 in text content
        QByteArray buffer;
        QXmlStreamWriter writer(&buffer);
        writer.writeStartDocument();
        writer.writeStartElement("root"_L1);
        writer.writeCharacters("Invalid \v character"_L1); // \v is invalid in XML 1.0
        writer.writeEndElement();
        writer.writeEndDocument();
        QVERIFY(writer.hasError());
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::InvalidCharacter);
        QVERIFY(!writer.errorString().isEmpty());
    }

    {
        // Invalid character error: forbidden control character for XML 1.0 U+0001
        QByteArray buffer;
        QXmlStreamWriter writer(&buffer);
        writer.writeStartDocument();
        writer.writeStartElement("root"_L1);
        writer.writeCharacters("Invalid \x01 character"_L1);
        writer.writeEndElement();
        writer.writeEndDocument();
        QVERIFY(writer.hasError());
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::InvalidCharacter);
        QVERIFY(!writer.errorString().isEmpty());
    }

    {
        // '\0' is an InvalidCharacter, not an EncodingError
        QByteArray buffer;
        QXmlStreamWriter writer(&buffer);
        writer.writeStartDocument();
        writer.writeStartElement("root"_L1);
        writer.writeCharacters("Invalid \0 character"_L1);
        writer.writeEndElement();
        writer.writeEndDocument();
        QVERIFY(writer.hasError());
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::InvalidCharacter);
        QVERIFY(!writer.errorString().isEmpty());
    }

    {
        // Custom error raised by user
        QByteArray buffer;
        QXmlStreamWriter writer(&buffer);
        writer.writeStartDocument();
        writer.writeStartElement("root"_L1);
        writer.raiseError("Custom error");
        writer.writeEndDocument();
        QVERIFY(writer.hasError());
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::Custom);
        QCOMPARE(writer.errorString(), "Custom error"_L1);
    }
}

void tst_QXmlStream::stopWritingOnError() const
{
    {
        // Default - stopWritingOnError(false)
        QByteArray buffer;
        QXmlStreamWriter writer(&buffer);
        writer.writeStartDocument();
        writer.writeStartElement(u"root");
        writer.writeCharacters(u"Invalid \x01 character");
        writer.writeTextElement(u"text", u"element");
        writer.writeComment(u"A comment");
        writer.writeEmptyElement(u"emptyElement");
        QVERIFY(writer.hasError());
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::InvalidCharacter);
        QVERIFY(!writer.errorString().isEmpty());
        QCOMPARE(buffer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                         "<root>Invalid  character<text>element</text>"
                         "<!--A comment--><emptyElement"_ba);

        writer.writeCharacters(u"Let's raise another error!");
        writer.raiseError(u"Custom error"_s);
        writer.writeEndElement();
        writer.writeTextElement(u"text", u"element");
        QVERIFY(writer.hasError());
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::Custom);
        QVERIFY(!writer.errorString().isEmpty());
        QCOMPARE(buffer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                         "<root>Invalid  character<text>element</text>"
                         "<!--A comment--><emptyElement/>"
                         "Let's raise another error!</root>"
                         "<text>element</text>"_ba);

        writer.writeStartElement(u"child");
        writer.writeCharacters(QChar(0xDC00));
        writer.writeCharacters(u"I'm still standin' better than I ever did!");
        writer.writeEndElement();
        writer.writeEndDocument();
        QVERIFY(writer.hasError());
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::Encoding);
        QVERIFY(!writer.errorString().isEmpty());
        QCOMPARE(buffer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                         "<root>Invalid  character<text>element</text>"
                         "<!--A comment--><emptyElement/>"
                         "Let's raise another error!</root><text>element</text>"
                         "<child>I'm still standin' better than I ever did!</child>\n"_ba);
    }

    {
        // Only IOError prevents further writing
        QByteArray buffer;
        QBuffer device(&buffer);
        device.open(QIODevice::WriteOnly);
        device.close();
        QXmlStreamWriter writer(&device);
        writer.setStopWritingOnError(false);
        writer.writeStartDocument();
        writer.writeStartElement(u"root");
        writer.writeCharacters(u"Some characters");
        QVERIFY(writer.hasError());
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::IO);
        QVERIFY(!writer.errorString().isEmpty());
        QVERIFY(buffer.isEmpty());
    }

    {
        // Valid input
        QByteArray buffer;
        QXmlStreamWriter writer(&buffer);
        writer.setStopWritingOnError(true);
        writer.writeStartDocument();
        writer.writeStartElement(u"root");
        writer.writeCharacters(u"Valid & possible to <escape> \"characters\"");
        writer.writeTextElement(u"text", u"element");
        writer.writeComment(u"A comment");
        writer.writeEmptyElement(u"emptyElement");
        writer.writeEndDocument();
        QVERIFY(!writer.hasError());
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::None);
        QVERIFY(writer.errorString().isEmpty());
        QCOMPARE(buffer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                         "<root>Valid &amp; possible to &lt;escape&gt; &quot;characters&quot;"
                         "<text>element</text><!--A comment--><emptyElement/>"
                         "</root>\n"_ba);
    }

    {
        // Invalid character error: invalid \x01 character
        QByteArray buffer;
        QXmlStreamWriter writer(&buffer);
        writer.setStopWritingOnError(true);
        writer.writeStartDocument();
        writer.writeStartElement(u"root");
        writer.writeCharacters(u"Invalid \x01 character"); // Stop writing from here
        writer.writeTextElement(u"text", u"element");
        writer.writeComment(u"A comment");
        writer.writeEmptyElement(u"emptyElement");
        writer.writeCDATA(u"CDATA");
        writer.writeEntityReference(u"entityReference");
        writer.writeProcessingInstruction(u"PI");
        writer.writeCharacters(u"Characters");
        writer.writeDTD(u"DTD");
        writer.writeDefaultNamespace(u"defaultNamespace");
        writer.writeNamespace(u"namespace");
        writer.writeEndElement();
        writer.writeEndDocument();
        QVERIFY(writer.hasError());
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::InvalidCharacter);
        QVERIFY(!writer.errorString().isEmpty());
        QCOMPARE(buffer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><root>"_ba);
    }

    {
        // Invalid character error: invalid \v character
        QByteArray buffer;
        QXmlStreamWriter writer(&buffer);
        writer.setStopWritingOnError(true);
        writer.writeStartDocument();
        writer.writeStartElement(u"root");
        writer.writeTextElement(u"text", u"element");
        writer.writeComment(u"A comment");
        writer.writeEmptyElement(u"emptyElement");
        writer.writeCDATA(u"CDATA");
        writer.writeEntityReference(u"entityReference");
        writer.writeProcessingInstruction(u"PI");
        writer.writeCharacters(u"Characters");
        writer.writeCharacters(u"Invalid \v character"); // Stop writing from here
        writer.writeCharacters(u"More valid characters");
        writer.writeDTD(u"DTD");
        writer.writeDefaultNamespace(u"defaultNamespace");
        writer.writeNamespace(u"namespace");
        writer.writeEndElement();
        writer.writeEndDocument();
        QVERIFY(writer.hasError());
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::InvalidCharacter);
        QVERIFY(!writer.errorString().isEmpty());
        QCOMPARE(buffer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                         "<root><text>element</text><!--A comment--><emptyElement/>"
                         "<![CDATA[CDATA]]>&entityReference;<?PI?>Characters"_ba);
    }

    {
        // Encoding error: lone low surrogate
        QByteArray buffer;
        QXmlStreamWriter writer(&buffer);
        writer.writeStartDocument();
        writer.setStopWritingOnError(true);
        writer.writeStartElement(u"root");
        writer.writeCharacters(QChar(0xDC00));  // Stop writing from here
        QVERIFY(writer.hasError());
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::Encoding);
        QVERIFY(!writer.errorString().isEmpty());
        QCOMPARE(buffer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><root>"_ba);

        writer.writeCharacters(u"I am a valid sentence");
        writer.writeCharacters(u"But I won't be written until the setting is changed.");
        writer.setStopWritingOnError(false);
        writer.writeCharacters(u"Resume writing!");
        writer.writeEndElement();
        writer.writeEndDocument();
        // Changing the flag doesn't clear the error; it just allows writing again.
        QVERIFY(writer.hasError());
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::Encoding);
        QVERIFY(!writer.errorString().isEmpty());
        QCOMPARE(buffer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                         "<root>Resume writing!</root>\n"_ba);

        writer.setStopWritingOnError(true);
        writer.writeCharacters(u"Valid characters rules!");
        QVERIFY(writer.hasError());
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::Encoding);
        QVERIFY(!writer.errorString().isEmpty());
        // Re-enabling stopWritingOnError does not clear the error state.
        // Since the writer is still in error, further writes are ignored even if valid.
        QCOMPARE(buffer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                         "<root>Resume writing!</root>\n"_ba);
    }

    {
        QByteArray buffer;
        QXmlStreamWriter writer(&buffer);
        writer.setStopWritingOnError(true);
        writer.writeStartDocument();
        writer.writeStartElement(u"root");
        writer.writeCharacters(u"Some characters");
        writer.raiseError(u"Raising custom error"_s);
        writer.writeCharacters(u"No more writing for you.");
        writer.writeEndElement();
        writer.writeEndDocument();
        QVERIFY(writer.hasError());
        QCOMPARE(writer.error(), QXmlStreamWriter::Error::Custom);
        QVERIFY(!writer.errorString().isEmpty());
        QCOMPARE(buffer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><root>Some characters"_ba);
    }
}

void tst_QXmlStream::invalidStringCharacters() const
{
    // test scan in attributes
    QFETCH(QString, testString);
    QFETCH(bool, expectedResultNoError);

    QByteArray values = testString.toUtf8();
    QBuffer inBuffer;
    inBuffer.setData(values);
    QVERIFY(inBuffer.open(QIODevice::ReadOnly));
    QXmlStreamReader reader(&inBuffer);
    do {
        reader.readNext();
    } while (!reader.atEnd());
    QCOMPARE((reader.error() == QXmlStreamReader::NoError), expectedResultNoError);
}

void tst_QXmlStream::invalidStringCharacters_data() const
{
    // test scan in attributes
    QTest::addColumn<bool>("expectedResultNoError");
    QTest::addColumn<QString>("testString");
    QChar ctrl(0x1A);
    QTest::newRow("utf8, attributes, legal") << true << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='aa'/>");
    QTest::newRow("utf8, attributes, only char, control") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='")+ctrl+QString("'/>");
    QTest::newRow("utf8, attributes, 1st char, control") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='")+ctrl+QString("abc'/>");
    QTest::newRow("utf8, attributes, middle char, control") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='abc")+ctrl+QString("efgx'/>");
    QTest::newRow("utf8, attributes, last char, control") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='abcde")+ctrl+QString("'/>");
    //
    QTest::newRow("utf8, text, legal") << true << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='aa'>abcx1A</root>");
    QTest::newRow("utf8, text, only, control") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='aa'>")+ctrl+QString("</root>");
    QTest::newRow("utf8, text, 1st char, control") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='aa'>abc")+ctrl+QString("def</root>");
    QTest::newRow("utf8, text, middle char, control") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='aa'>abc")+ctrl+QString("efg</root>");
    QTest::newRow("utf8, text, last char, control") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='aa'>abc")+ctrl+QString("</root>");
    //
    QTest::newRow("utf8, cdata text, legal") << true << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='aa'><![CDATA[abcdefghi]]></root>");
    QTest::newRow("utf8, cdata text, only, control") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='aa'><![CDATA[")+ctrl+QString("]]></root>");
    QTest::newRow("utf8, cdata text, 1st char, control") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='aa'><![CDATA[")+ctrl+QString("abcdefghi]]></root>");
    QTest::newRow("utf8, cdata text, middle char, control") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='aa'><![CDATA[abcd")+ctrl+QString("efghi]]></root>");
    QTest::newRow("utf8, cdata text, last char, control") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='aa'><![CDATA[abcdefghi")+ctrl+QString("]]></root>");
    //
    QTest::newRow("utf8, mixed, control") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='a")+ctrl+QString("a'><![CDATA[abcdefghi")+ctrl+QString("]]></root>");
    QTest::newRow("utf8, tag") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><roo")+ctrl+QString("t attr='aa'><![CDATA[abcdefghi]]></roo")+ctrl+QString("t>");
    //
    QTest::newRow("utf8, attributes, 1st char, legal escaping hex") << true << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='a&#xA0;'/>");
    QTest::newRow("utf8, attributes, 1st char, control escaping hex") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='&#x1A;aaa'/>");
    QTest::newRow("utf8, attributes, middle char, legal escaping hex") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='aaa&#x1A;aaa'/>");
    QTest::newRow("utf8, attributes, last char, control escaping hex") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='aaa&#x1A;'/>");
    QTest::newRow("utf8, attributes, 1st char, legal escaping dec") << true << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='a&#160;'/>");
    QTest::newRow("utf8, attributes, 1st char, control escaping dec") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='&#26;aaaa'/>");
    QTest::newRow("utf8, attributes, middle char, legal escaping dec") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='aaa&#26;aaaaa'/>");
    QTest::newRow("utf8, attributes, last char, control escaping dec") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='aaaaaa&#26;'/>");
    QTest::newRow("utf8, tag escaping") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><roo&#x1A;t attr='aa'><![CDATA[abcdefghi]]></roo&#x1A;t>");
    //
    QTest::newRow("utf8, mix of illegal control") << false << QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr='a&#0;&#x4;&#x1c;a'><![CDATA[abcdefghi]]></root>");
    //
}

static bool isValidSingleTextChar(char32_t c)
{
    // Conforms to https://www.w3.org/TR/REC-xml/#NT-Char
    // Char ::= #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF]
    constexpr struct { char32_t lo, hi; } validRanges[] = {
        {0x9, 0xA},
        {0xD, 0xD},
        {0x20, 0xD7ff},
        {0xE000, 0xFFFD},
        {0x1'0000, 0x10'FFFF},
    };

    for (const auto range : validRanges) {
        if (c >= range.lo && c <= range.hi)
            return true;
    }
    return false;
}

void tst_QXmlStream::readBack_data() const
{
    QTest::addColumn<int>("plane");

    // Check all 17 Unicode planes. Split into separate executions lest the
    // test function times out in asan builds.

    for (int i = 0; i < 17; ++i)
        QTest::addRow("plane-%02d", i) << i;
}

void tst_QXmlStream::readBack() const
{
    QFETCH(const int, plane);

    constexpr qsizetype MaxChunkSizeWhenEncoding = 512; // from qxmlstream.cpp
    QBuffer buffer;
    QString text = QString(513, 'a'); // one longer than the internal conversion buffer

    for (char16_t i = 0; i < (std::numeric_limits<char16_t>::max)(); ++i) {

        const char32_t c = (plane << 16) + i;

        // end chunk in invalid character, split surrogates:
        const auto pair = QChar::fromUcs4(c);
        text.resize(MaxChunkSizeWhenEncoding + 1 - pair.size());
        text += pair;

        QVERIFY(buffer.open(QIODevice::WriteOnly|QIODevice::Truncate));
        QXmlStreamWriter writer(&buffer);
        writer.writeStartDocument();
        writer.writeTextElement("a", text);
        writer.writeEndDocument();
        buffer.close();

        if (!isValidSingleTextChar(c)) {
            QVERIFY2(writer.hasError(), QByteArray::number(c));
        } else {
            QVERIFY2(!writer.hasError(), QByteArray::number(c));
            QVERIFY(buffer.open(QIODevice::ReadOnly));
            QXmlStreamReader reader(&buffer);
            do {
                reader.readNext();
            } while (!reader.atEnd());
            QVERIFY2(!reader.hasError(), QByteArray::number(c));
        }
    }
}

void tst_QXmlStream::roundTrip_data() const
{
    QTest::addColumn<QString>("in");

    QTest::newRow("QTBUG-63434") <<
        "<?xml version=\"1.0\"?>"
        "<root>"
            "<father>"
                "<child xmlns:unknown=\"http://mydomain\">Text</child>"
            "</father>"
        "</root>\n";

    // When a namespace is introduced by an attribute of an element,
    // that element can exercise the namespace in its tag.
    // This used (QTBUG-75456) to lead to the namespace definition
    // being wrongly duplicated, with a new name.
    QTest::newRow("QTBUG-75456") <<
        "<?xml version=\"1.0\"?>"
        "<abc:root xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:abc=\"ns1\">"
            "<abc:parent>"
                "<abc:child xmlns:unknown=\"http://mydomain\">Text</abc:child>"
            "</abc:parent>"
            "<def:parent xmlns:def=\"ns2\" id=\"test\">"
                "<def:child id=\"Timmy\">More text</def:child>"
                "<def:child id=\"Jimmy\">Even more text</def:child>"
            "</def:parent>"
        "</abc:root>\n";
}

void tst_QXmlStream::entityExpansionLimit() const
{
    QString xml = QStringLiteral("<?xml version=\"1.0\"?>"
                                 "<!DOCTYPE foo ["
                                 "<!ENTITY a \"0123456789\" >"
                                 "<!ENTITY b \"&a;&a;&a;&a;&a;&a;&a;&a;&a;&a;\" >"
                                 "<!ENTITY c \"&b;&b;&b;&b;&b;&b;&b;&b;&b;&b;\" >"
                                 "<!ENTITY d \"&c;&c;&c;&c;&c;&c;&c;&c;&c;&c;\" >"
                                 "]>"
                                 "<foo>&d;&d;&d;</foo>");
    {
        QXmlStreamReader reader(xml);
        QCOMPARE(reader.entityExpansionLimit(), 4096);
        do {
            reader.readNext();
        } while (!reader.atEnd());
        QCOMPARE(reader.error(), QXmlStreamReader::NotWellFormedError);
    }

    // &d; expands to 10k characters, minus the 3 removed (&d;) means it should fail
    // with a limit of 9996 chars and pass with 9997
    {
        QXmlStreamReader reader(xml);
        reader.setEntityExpansionLimit(9996);
        do {
            reader.readNext();
        } while (!reader.atEnd());

        QCOMPARE(reader.error(), QXmlStreamReader::NotWellFormedError);
    }
    {
        QXmlStreamReader reader(xml);
        reader.setEntityExpansionLimit(9997);
        do {
            reader.readNext();
        } while (!reader.atEnd());
        QCOMPARE(reader.error(), QXmlStreamReader::NoError);
    }
}

void tst_QXmlStream::roundTrip() const
{
    QFETCH(QString, in);
    QString out;

    QXmlStreamReader reader(in);
    QXmlStreamWriter writer(&out);

    while (!reader.atEnd()) {
        reader.readNext();
        QVERIFY(!reader.hasError());
        writer.writeCurrentToken(reader);
        QVERIFY(!writer.hasError());
    }
    QCOMPARE(out, in);
}

void tst_QXmlStream::test_fastScanName_data() const
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QXmlStreamReader::Error>("errorType");

    // 4096 is the limit in QXmlStreamReaderPrivate::fastScanName()

    QByteArray arr = "<a:" + QByteArray("b").repeated(4096 - 1);
    QTest::newRow("data1") << arr << QXmlStreamReader::PrematureEndOfDocumentError;

    arr = "<a:" + QByteArray("b").repeated(4096);
    QTest::newRow("data2") << arr << QXmlStreamReader::NotWellFormedError;

    arr = "<" + QByteArray("a").repeated(4000) + ":" + QByteArray("b").repeated(96);
    QTest::newRow("data3") << arr << QXmlStreamReader::PrematureEndOfDocumentError;

    arr = "<" + QByteArray("a").repeated(4000) + ":" + QByteArray("b").repeated(96 + 1);
    QTest::newRow("data4") << arr << QXmlStreamReader::NotWellFormedError;

    arr = "<" + QByteArray("a").repeated(4000 + 1) + ":" + QByteArray("b").repeated(96);
    QTest::newRow("data5") << arr << QXmlStreamReader::NotWellFormedError;
}

void tst_QXmlStream::test_fastScanName() const
{
    QFETCH(QByteArray, data);
    QFETCH(QXmlStreamReader::Error, errorType);

    QXmlStreamReader reader(data);
    QXmlStreamReader::TokenType tokenType;
    while (!reader.atEnd())
        tokenType = reader.readNext();

    QCOMPARE(tokenType, QXmlStreamReader::Invalid);
    QCOMPARE(reader.error(), errorType);
}

void tst_QXmlStream::tokenErrorHandling_data() const
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QXmlStreamReader::Error>("expectedError");
    QTest::addColumn<QString>("errorKeyWord");

    constexpr auto invalid = QXmlStreamReader::Error::UnexpectedElementError;
    constexpr auto valid = QXmlStreamReader::Error::NoError;
    QTest::newRow("DtdInBody") << "dtdInBody.xml" << invalid << "DTD";
    QTest::newRow("multipleDTD") << "multipleDtd.xml" << invalid << "second DTD";
    QTest::newRow("wellFormed") << "wellFormed.xml" << valid << "";
}

void tst_QXmlStream::tokenErrorHandling() const
{
    QFETCH(const QString, fileName);
    QFETCH(const QXmlStreamReader::Error, expectedError);
    QFETCH(const QString, errorKeyWord);

    const QDir dir(QFINDTESTDATA("tokenError"));
    QFile file(dir.absoluteFilePath(fileName));

    // Cross-compiling: Files may not be found when running test standalone
    // QSKIP in that case, because the tested functionality is platform independent.
    if (!file.exists())
        QSKIP(QObject::tr("Testfile %1 not found.").arg(fileName).toUtf8().constData());

    QVERIFY(file.open(QIODevice::ReadOnly));
    QXmlStreamReader reader(&file);
    while (!reader.atEnd())
        reader.readNext();

    QCOMPARE(reader.error(), expectedError);
    if (expectedError != QXmlStreamReader::Error::NoError)
        QVERIFY(reader.errorString().contains(errorKeyWord));
}

void tst_QXmlStream::checkStreamNotationDeclarations() const
{
    QString fileName("12.xml");
    const QDir dir(QFINDTESTDATA("data"));
    QFile file(dir.absoluteFilePath(fileName));
    if (!file.exists())
        QSKIP(QObject::tr("Testfile %1 not found.").arg(fileName).toUtf8().constData());
    QVERIFY(file.open(QIODevice::ReadOnly));
    QXmlStreamReader reader(&file);
    while (!reader.atEnd())
        reader.readNext();

    QVERIFY(!reader.hasError());
    QXmlStreamNotationDeclaration notation1, notation2, notation3;
    QT_TEST_EQUALITY_OPS(notation1, notation2, true);
    const auto notationDeclarations = reader.notationDeclarations();
    if (notationDeclarations.count() >= 2) {
        notation1 = notationDeclarations.at(0);
        notation2 = notationDeclarations.at(1);
        notation3 = notationDeclarations.at(1);
    }
    QT_TEST_EQUALITY_OPS(notation1, notation2, false);
    QT_TEST_EQUALITY_OPS(notation3, notation2, true);
}

void tst_QXmlStream::checkStreamEntityDeclarations() const
{
    QString fileName("5.xml");
    const QDir dir(QFINDTESTDATA("data"));
    QFile file(dir.absoluteFilePath(fileName));
    if (!file.exists())
        QSKIP(QObject::tr("Testfile %1 not found.").arg(fileName).toUtf8().constData());
    QVERIFY(file.open(QIODevice::ReadOnly));
    QXmlStreamReader reader(&file);
    while (!reader.atEnd())
        reader.readNext();

    QVERIFY(!reader.hasError());
    QXmlStreamEntityDeclaration entity;
    QT_TEST_EQUALITY_OPS(entity, QXmlStreamEntityDeclaration(), true);

    const auto entityDeclarations = reader.entityDeclarations();
    if (entityDeclarations.count() >= 2) {
        entity = entityDeclarations.at(1);
        QT_TEST_EQUALITY_OPS(entityDeclarations.at(0), entityDeclarations.at(1), false);
        QT_TEST_EQUALITY_OPS(entity, entityDeclarations.at(1), true);
    }
}
#include "tst_qxmlstream.moc"
