// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#ifdef Q_OS_UNIX
#include <locale.h>
#endif

#include <QBuffer>
#include <QByteArray>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryFile>
#include <QStringConverter>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTextStream>
#if QT_CONFIG(process)
# include <QProcess>
#endif
#include "../../../network-settings.h"
#include <QtTest/private/qemulationdetector_p.h>

using namespace Qt::StringLiterals;

QT_BEGIN_NAMESPACE
template<> struct QMetaTypeId<QIODevice::OpenModeFlag>
{ enum { Defined = 1 }; static inline int qt_metatype_id() { return QMetaType::Int; } };
QT_END_NAMESPACE

class tst_QTextStream : public QObject
{
    Q_OBJECT

public:
    tst_QTextStream();

public slots:
    void initTestCase();
    void cleanup();
    void cleanupTestCase();

private slots:
    void getSetCheck();
    void construction();

    // lines
    void readLineFromDevice_data() { generateLineData(false); }
    void readLineFromDevice();
    void readLineFromString_data() { generateLineData(true); }
    void readLineFromString();
    void readLineFromTextDevice_data() { generateLineData(false); }
    void readLineFromTextDevice();
    void readLineUntilNull();
    void readLineMaxlen_data();
    void readLineMaxlen();
    void readLinesFromBufferCRCR();
    void readLineInto();

    // all
    void readAllFromDevice_data() { generateAllData(false); }
    void readAllFromDevice();
    void readAllFromString_data() { generateAllData(true); }
    void readAllFromString();
    void readLineFromStringThenChangeString();

    // device tests
    void setDevice();

    // char operators
    void QChar_operators_FromDevice_data() { generateOperatorCharData(false); }
    void QChar_operators_FromDevice();
    void char16_t_operators_FromDevice_data() { generateOperatorCharData(false); }
    void char16_t_operators_FromDevice();
    void char_operators_FromDevice_data() { generateOperatorCharData(false); }
    void char_operators_FromDevice();

    // natural number read operator
    void signedShort_read_operator_FromDevice_data() { generateNaturalNumbersData(false); }
    void signedShort_read_operator_FromDevice() { integral_read_operator_FromDevice<signed short>(); }
    void unsignedShort_read_operator_FromDevice_data() { generateNaturalNumbersData(false); }
    void unsignedShort_read_operator_FromDevice() { integral_read_operator_FromDevice<unsigned short>(); }
    void signedInt_read_operator_FromDevice_data() { generateNaturalNumbersData(false); }
    void signedInt_read_operator_FromDevice() { integral_read_operator_FromDevice<signed int>(); }
    void unsignedInt_read_operator_FromDevice_data() { generateNaturalNumbersData(false); }
    void unsignedInt_read_operator_FromDevice() { integral_read_operator_FromDevice<unsigned int>(); }
    void qlonglong_read_operator_FromDevice_data() { generateNaturalNumbersData(false); }
    void qlonglong_read_operator_FromDevice() { integral_read_operator_FromDevice<qlonglong>(); }
    void qulonglong_read_operator_FromDevice_data() { generateNaturalNumbersData(false); }
    void qulonglong_read_operator_FromDevice() { integral_read_operator_FromDevice<qulonglong>(); }

    // natural number write operator
    void signedShort_write_operator_ToDevice_data();
    void signedShort_write_operator_ToDevice() { integral_write_operator_ToDevice<signed short>(); }
    void unsignedShort_write_operator_ToDevice_data();
    void unsignedShort_write_operator_ToDevice() { integral_write_operator_ToDevice<unsigned short>(); }
    void signedInt_write_operator_ToDevice_data();
    void signedInt_write_operator_ToDevice() { integral_write_operator_ToDevice<signed int>(); }
    void unsignedInt_write_operator_ToDevice_data();
    void unsignedInt_write_operator_ToDevice() { integral_write_operator_ToDevice<unsigned int>(); }
    void qlonglong_write_operator_ToDevice_data();
    void qlonglong_write_operator_ToDevice() { integral_write_operator_ToDevice<qlonglong>(); }
    void qulonglong_write_operator_ToDevice_data();
    void qulonglong_write_operator_ToDevice() { integral_write_operator_ToDevice<qulonglong>(); }

    void int_read_with_locale_data();
    void int_read_with_locale();

    void int_write_with_locale_data();
    void int_write_with_locale();

    // real number read operator
    void float_read_operator_FromDevice_data() { generateRealNumbersData(false); }
    void float_read_operator_FromDevice() { real_read_operator_FromDevice<float>(); }
    void double_read_operator_FromDevice_data() { generateRealNumbersData(false); }
    void double_read_operator_FromDevice() { real_read_operator_FromDevice<double>(); }

    // real number write operator
    void float_write_operator_ToDevice_data() { generateRealNumbersDataWrite(); }
    void float_write_operator_ToDevice() { real_write_operator_ToDevice<float>(); }
    void double_write_operator_ToDevice_data() { generateRealNumbersDataWrite(); }
    void double_write_operator_ToDevice() { real_write_operator_ToDevice<double>(); }

    void double_write_with_flags_data();
    void double_write_with_flags();

    void double_write_with_precision_data();
    void double_write_with_precision();

    // text read operators
    void charPtr_read_operator_FromDevice_data() { generateStringData(false); }
    void charPtr_read_operator_FromDevice();
    void stringRef_read_operator_FromDevice_data() { generateStringData(false); }
    void stringRef_read_operator_FromDevice();
    void byteArray_read_operator_FromDevice_data() { generateStringData(false); }
    void byteArray_read_operator_FromDevice();

    // text write operators
    void string_write_operator_ToDevice_data();
    void string_write_operator_ToDevice();
    void latin1String_write_operator_ToDevice();
    void stringref_write_operator_ToDevice();
    void stringview_write_operator_ToDevice();

    // bool operator
    void stream_bool_operator_Test();

    // other
    void skipWhiteSpace_data();
    void skipWhiteSpace();
    void lineCount_data();
    void lineCount();
    void performance();
    void hexTest_data();
    void hexTest();
    void binTest_data();
    void binTest();
    void octTest_data();
    void octTest();
    void zeroTermination();
    void ws_manipulator();
    void stillOpenWhenAtEnd();
    void readNewlines_data();
    void readNewlines();
    void seek();
    void pos();
    void pos2();
    void pos3LargeFile();
    void readStdin();
    void readAllFromStdin();
    void readLineFromStdin();
    void read();
    void qbool();
    void forcePoint();
    void forceSign();
    void read0d0d0a();
    void numeralCase_data();
    void numeralCase();
    void nanInf();
    void utf8IncompleteAtBufferBoundary_data();
    void utf8IncompleteAtBufferBoundary();
    void writeSeekWriteNoBOM();

    // status
    void status_real_read_data();
    void status_real_read();
    void status_integer_read();
    void status_word_read();
    void status_write_error();

    // use case tests
    void useCase1();
    void useCase2();

    // manipulators
    void manipulators_data();
    void manipulators();

    // UTF-16 BOM (Byte Order Mark)
    void generateBOM();
    void readBomSeekBackReadBomAgain();

    // Regression tests for old bugs
    void alignAccountingStyle();
    void setEncoding();

    void textModeOnEmptyRead();

    void autodetectUnicode_data();
    void autodetectUnicode();

private:
    void generateLineData(bool for_QString) const;
    void generateAllData(bool for_QString) const;
    void generateOperatorCharData(bool for_QString) const;
    template <typename Whole> void integral_read_operator_FromDevice() const;
    void generateNaturalNumbersData(bool for_QString) const;
    template <typename Whole> void integral_write_operator_ToDevice() const;
    template <typename Real> void real_read_operator_FromDevice() const;
    void generateRealNumbersData(bool for_QString) const;
    void generateStringData(bool for_QString) const;
    template <typename Real> void real_write_operator_ToDevice() const;
    void generateRealNumbersDataWrite() const;

    QTemporaryDir tempDir;
    QString testFileName;
#ifdef BUILTIN_TESTDATA
    QSharedPointer<QTemporaryDir> m_dataDir;
#endif
    const QString m_rfc3261FilePath;
};

void runOnExit()
{
    QByteArray buffer;
    QTextStream(&buffer) << "This will try to use QStringConverter::Utf8" << Qt::endl;
}
Q_DESTRUCTOR_FUNCTION(runOnExit)

tst_QTextStream::tst_QTextStream()
    : tempDir(QDir::tempPath() + "/tst_qtextstream.XXXXXX")
    , m_rfc3261FilePath(QFINDTESTDATA("rfc3261.txt"))
{
}

void tst_QTextStream::initTestCase()
{
    QVERIFY2(tempDir.isValid(), qPrintable(tempDir.errorString()));
    QVERIFY(!m_rfc3261FilePath.isEmpty());

    testFileName = tempDir.path() + "/testfile";

#ifdef BUILTIN_TESTDATA
    m_dataDir = QEXTRACTTESTDATA("/");
    QVERIFY2(QDir::setCurrent(m_dataDir->path()), qPrintable("Could not chdir to " + m_dataDir->path()));
#else
    // chdir into the testdata dir and refer to our helper apps with relative paths
    QString testdata_dir = QFileInfo(QFINDTESTDATA("stdinProcess")).absolutePath();
    QVERIFY2(QDir::setCurrent(testdata_dir), qPrintable("Could not chdir to " + testdata_dir));
#endif
}

// Testing get/set functions
void tst_QTextStream::getSetCheck()
{
    // Initialize codecs
    QTextStream obj1;
    // QTextStream::encoding()
    // QTextStream::setEncoding()
    obj1.setEncoding(QStringConverter::Utf32BE);
    QCOMPARE(QStringConverter::Utf32BE, obj1.encoding());
    obj1.setEncoding(QStringConverter::Utf8);
    QCOMPARE(QStringConverter::Utf8, obj1.encoding());

    // bool QTextStream::autoDetectUnicode()
    // void QTextStream::setAutoDetectUnicode(bool)
    obj1.setAutoDetectUnicode(false);
    QCOMPARE(false, obj1.autoDetectUnicode());
    obj1.setAutoDetectUnicode(true);
    QCOMPARE(true, obj1.autoDetectUnicode());

    // bool QTextStream::generateByteOrderMark()
    // void QTextStream::setGenerateByteOrderMark(bool)
    obj1.setGenerateByteOrderMark(false);
    QCOMPARE(false, obj1.generateByteOrderMark());
    obj1.setGenerateByteOrderMark(true);
    QCOMPARE(true, obj1.generateByteOrderMark());

    // QIODevice * QTextStream::device()
    // void QTextStream::setDevice(QIODevice *)
    QFile *var4 = new QFile;
    obj1.setDevice(var4);
    QCOMPARE(static_cast<QIODevice *>(var4), obj1.device());
    obj1.setDevice((QIODevice *)0);
    QCOMPARE((QIODevice *)0, obj1.device());
    delete var4;

    // Status QTextStream::status()
    // void QTextStream::setStatus(Status)
    obj1.setStatus(QTextStream::Status(QTextStream::Ok));
    QCOMPARE(QTextStream::Status(QTextStream::Ok), obj1.status());
    obj1.setStatus(QTextStream::Status(QTextStream::ReadPastEnd));
    QCOMPARE(QTextStream::Status(QTextStream::ReadPastEnd), obj1.status());
    obj1.resetStatus();
    obj1.setStatus(QTextStream::Status(QTextStream::ReadCorruptData));
    QCOMPARE(QTextStream::Status(QTextStream::ReadCorruptData), obj1.status());

    // FieldAlignment QTextStream::fieldAlignment()
    // void QTextStream::setFieldAlignment(FieldAlignment)
    obj1.setFieldAlignment(QTextStream::FieldAlignment(QTextStream::AlignLeft));
    QCOMPARE(QTextStream::FieldAlignment(QTextStream::AlignLeft), obj1.fieldAlignment());
    obj1.setFieldAlignment(QTextStream::FieldAlignment(QTextStream::AlignRight));
    QCOMPARE(QTextStream::FieldAlignment(QTextStream::AlignRight), obj1.fieldAlignment());
    obj1.setFieldAlignment(QTextStream::FieldAlignment(QTextStream::AlignCenter));
    QCOMPARE(QTextStream::FieldAlignment(QTextStream::AlignCenter), obj1.fieldAlignment());
    obj1.setFieldAlignment(QTextStream::FieldAlignment(QTextStream::AlignAccountingStyle));
    QCOMPARE(QTextStream::FieldAlignment(QTextStream::AlignAccountingStyle), obj1.fieldAlignment());

    // QChar QTextStream::padChar()
    // void QTextStream::setPadChar(QChar)
    QChar var7 = 'Q';
    obj1.setPadChar(var7);
    QCOMPARE(var7, obj1.padChar());
    obj1.setPadChar(QChar());
    QCOMPARE(QChar(), obj1.padChar());

    // int QTextStream::fieldWidth()
    // void QTextStream::setFieldWidth(int)
    obj1.setFieldWidth(0);
    QCOMPARE(0, obj1.fieldWidth());
    obj1.setFieldWidth(INT_MIN);
    QCOMPARE(INT_MIN, obj1.fieldWidth());
    obj1.setFieldWidth(INT_MAX);
    QCOMPARE(INT_MAX, obj1.fieldWidth());

    // NumberFlags QTextStream::numberFlags()
    // void QTextStream::setNumberFlags(NumberFlags)
    obj1.setNumberFlags(QTextStream::NumberFlags(QTextStream::ShowBase));
    QCOMPARE(QTextStream::NumberFlags(QTextStream::ShowBase), obj1.numberFlags());
    obj1.setNumberFlags(QTextStream::NumberFlags(QTextStream::ForcePoint));
    QCOMPARE(QTextStream::NumberFlags(QTextStream::ForcePoint), obj1.numberFlags());
    obj1.setNumberFlags(QTextStream::NumberFlags(QTextStream::ForceSign));
    QCOMPARE(QTextStream::NumberFlags(QTextStream::ForceSign), obj1.numberFlags());
    obj1.setNumberFlags(QTextStream::NumberFlags(QTextStream::UppercaseBase));
    QCOMPARE(QTextStream::NumberFlags(QTextStream::UppercaseBase), obj1.numberFlags());
    obj1.setNumberFlags(QTextStream::NumberFlags(QTextStream::UppercaseDigits));
    QCOMPARE(QTextStream::NumberFlags(QTextStream::UppercaseDigits), obj1.numberFlags());

    // int QTextStream::integerBase()
    // void QTextStream::setIntegerBase(int)
    obj1.setIntegerBase(0);
    QCOMPARE(0, obj1.integerBase());
    obj1.setIntegerBase(INT_MIN);
    QCOMPARE(INT_MIN, obj1.integerBase());
    obj1.setIntegerBase(INT_MAX);
    QCOMPARE(INT_MAX, obj1.integerBase());

    // RealNumberNotation QTextStream::realNumberNotation()
    // void QTextStream::setRealNumberNotation(RealNumberNotation)
    obj1.setRealNumberNotation(QTextStream::RealNumberNotation(QTextStream::SmartNotation));
    QCOMPARE(QTextStream::RealNumberNotation(QTextStream::SmartNotation), obj1.realNumberNotation());
    obj1.setRealNumberNotation(QTextStream::RealNumberNotation(QTextStream::FixedNotation));
    QCOMPARE(QTextStream::RealNumberNotation(QTextStream::FixedNotation), obj1.realNumberNotation());
    obj1.setRealNumberNotation(QTextStream::RealNumberNotation(QTextStream::ScientificNotation));
    QCOMPARE(QTextStream::RealNumberNotation(QTextStream::ScientificNotation), obj1.realNumberNotation());

    // int QTextStream::realNumberPrecision()
    // void QTextStream::setRealNumberPrecision(int)
    obj1.setRealNumberPrecision(0);
    QCOMPARE(0, obj1.realNumberPrecision());
    obj1.setRealNumberPrecision(INT_MIN);
    QCOMPARE(6, obj1.realNumberPrecision()); // Setting a negative precision reverts it to the default value (6).
    obj1.setRealNumberPrecision(INT_MAX);
    QCOMPARE(INT_MAX, obj1.realNumberPrecision());
}

void tst_QTextStream::cleanup()
{
    QCoreApplication::instance()->processEvents();
}

void tst_QTextStream::cleanupTestCase()
{
#ifdef BUILTIN_TESTDATA
    QDir::setCurrent(QCoreApplication::applicationDirPath());
#endif
}

// ------------------------------------------------------------------------------
void tst_QTextStream::construction()
{
    QTextStream stream;
    QCOMPARE(stream.encoding(), QStringConverter::Utf8);
    QCOMPARE(stream.device(), static_cast<QIODevice *>(0));
    QCOMPARE(stream.string(), static_cast<QString *>(0));

    QTest::ignoreMessage(QtWarningMsg, "QTextStream: No device");
    QVERIFY(stream.atEnd());

    QTest::ignoreMessage(QtWarningMsg, "QTextStream: No device");
    QCOMPARE(stream.readAll(), QString());

}

void tst_QTextStream::generateLineData(bool for_QString) const
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QStringList>("lines");

    // latin-1
    QTest::newRow("emptyer") << QByteArray() << QStringList();
    QTest::newRow("lf") << QByteArray("\n") << (QStringList() << "");
    QTest::newRow("crlf") << QByteArray("\r\n") << (QStringList() << "");
    QTest::newRow("oneline/nothing") << QByteArray("ole") << (QStringList() << "ole");
    QTest::newRow("oneline/lf") << QByteArray("ole\n") << (QStringList() << "ole");
    QTest::newRow("oneline/crlf") << QByteArray("ole\r\n") << (QStringList() << "ole");
    QTest::newRow("twolines/lf/lf") << QByteArray("ole\ndole\n") << (QStringList() << "ole" << "dole");
    QTest::newRow("twolines/crlf/crlf") << QByteArray("ole\r\ndole\r\n") << (QStringList() << "ole" << "dole");
    QTest::newRow("twolines/lf/crlf") << QByteArray("ole\ndole\r\n") << (QStringList() << "ole" << "dole");
    QTest::newRow("twolines/lf/nothing") << QByteArray("ole\ndole") << (QStringList() << "ole" << "dole");
    QTest::newRow("twolines/crlf/nothing") << QByteArray("ole\r\ndole") << (QStringList() << "ole" << "dole");
    QTest::newRow("threelines/lf/lf/lf") << QByteArray("ole\ndole\ndoffen\n") << (QStringList() << "ole" << "dole" << "doffen");
    QTest::newRow("threelines/crlf/crlf/crlf") << QByteArray("ole\r\ndole\r\ndoffen\r\n") << (QStringList() << "ole" << "dole" << "doffen");
    QTest::newRow("threelines/crlf/crlf/nothing") << QByteArray("ole\r\ndole\r\ndoffen") << (QStringList() << "ole" << "dole" << "doffen");

    if (!for_QString) {
        // utf-8
        QTest::newRow("utf8/twolines")
            << QByteArray("\xef\xbb\xbf"
                          "\x66\x67\x65\x0a"
                          "\x66\x67\x65\x0a", 11)
            << (QStringList() << "fge" << "fge");

        // utf-16
        // one line
        QTest::newRow("utf16-BE/nothing")
            << QByteArray("\xfe\xff"
                          "\x00\xe5\x00\x67\x00\x65", 8) << (QStringList() << QLatin1String("\345ge"));
        QTest::newRow("utf16-LE/nothing")
            << QByteArray("\xff\xfe"
                          "\xe5\x00\x67\x00\x65\x00", 8) << (QStringList() << QLatin1String("\345ge"));
        QTest::newRow("utf16-BE/lf")
            << QByteArray("\xfe\xff"
                          "\x00\xe5\x00\x67\x00\x65\x00\x0a", 10) << (QStringList() << QLatin1String("\345ge"));
        QTest::newRow("utf16-LE/lf")
            << QByteArray("\xff\xfe"
                          "\xe5\x00\x67\x00\x65\x00\x0a\x00", 10) << (QStringList() << QLatin1String("\345ge"));

        // two lines
        QTest::newRow("utf16-BE/twolines")
            << QByteArray("\xfe\xff"
                          "\x00\xe5\x00\x67\x00\x65\x00\x0a"
                          "\x00\xe5\x00\x67\x00\x65\x00\x0a", 18)
            << (QStringList() << QLatin1String("\345ge") << QLatin1String("\345ge"));
        QTest::newRow("utf16-LE/twolines")
            << QByteArray("\xff\xfe"
                          "\xe5\x00\x67\x00\x65\x00\x0a\x00"
                          "\xe5\x00\x67\x00\x65\x00\x0a\x00", 18)
            << (QStringList() << QLatin1String("\345ge") << QLatin1String("\345ge"));

        // three lines
        QTest::newRow("utf16-BE/threelines")
            << QByteArray("\xfe\xff"
                          "\x00\xe5\x00\x67\x00\x65\x00\x0a"
                          "\x00\xe5\x00\x67\x00\x65\x00\x0a"
                          "\x00\xe5\x00\x67\x00\x65\x00\x0a", 26)
            << (QStringList() << QLatin1String("\345ge") << QLatin1String("\345ge") << QLatin1String("\345ge"));
        QTest::newRow("utf16-LE/threelines")
            << QByteArray("\xff\xfe"
                          "\xe5\x00\x67\x00\x65\x00\x0a\x00"
                          "\xe5\x00\x67\x00\x65\x00\x0a\x00"
                          "\xe5\x00\x67\x00\x65\x00\x0a\x00", 26)
            << (QStringList() << QLatin1String("\345ge") << QLatin1String("\345ge") << QLatin1String("\345ge"));

        // utf-32
        QTest::newRow("utf32-BE/twolines")
            << QByteArray("\x00\x00\xfe\xff"
                          "\x00\x00\x00\xe5\x00\x00\x00\x67\x00\x00\x00\x65\x00\x00\x00\x0a"
                          "\x00\x00\x00\xe5\x00\x00\x00\x67\x00\x00\x00\x65\x00\x00\x00\x0a", 36)
            << (QStringList() << QLatin1String("\345ge") << QLatin1String("\345ge"));
        QTest::newRow("utf32-LE/twolines")
            << QByteArray("\xff\xfe\x00\x00"
                          "\xe5\x00\x00\x00\x67\x00\x00\x00\x65\x00\x00\x00\x0a\x00\x00\x00"
                          "\xe5\x00\x00\x00\x67\x00\x00\x00\x65\x00\x00\x00\x0a\x00\x00\x00", 36)
            << (QStringList() << QLatin1String("\345ge") << QLatin1String("\345ge"));
    }

    // partials
    QTest::newRow("cr") << QByteArray("\r") << (QStringList() << "");
    QTest::newRow("oneline/cr") << QByteArray("ole\r") << (QStringList() << "ole");
    if (!for_QString)
        QTest::newRow("utf16-BE/cr") << QByteArray("\xfe\xff\x00\xe5\x00\x67\x00\x65\x00\x0d", 10) << (QStringList() << QLatin1String("\345ge"));
}

// ------------------------------------------------------------------------------
void tst_QTextStream::readLineFromDevice()
{
    QFETCH(QByteArray, data);
    QFETCH(QStringList, lines);

    QFile::remove(testFileName);
    QFile file(testFileName);
    QVERIFY(file.open(QFile::ReadWrite));
    QCOMPARE(file.write(data), qlonglong(data.size()));
    QVERIFY(file.flush());
    file.seek(0);

    QTextStream stream(&file);
    QStringList list;
    while (!stream.atEnd())
        list << stream.readLine();

    QCOMPARE(list, lines);
}

// ------------------------------------------------------------------------------
void tst_QTextStream::readLineMaxlen_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QStringList>("lines");

    QTest::newRow("Hey")
        << QString("Hey")
        << (QStringList() << QString("Hey") << QString(""));
    QTest::newRow("Hey\\n")
        << QString("Hey\n")
        << (QStringList() << QString("Hey") << QString(""));
    QTest::newRow("HelloWorld")
        << QString("HelloWorld")
        << (QStringList() << QString("Hello") << QString("World"));
    QTest::newRow("Helo\\nWorlds")
        << QString("Helo\nWorlds")
        << (QStringList() << QString("Helo") << QString("World"));
    QTest::newRow("AAAAA etc.")
        << QString(16385, QLatin1Char('A'))
        << (QStringList() << QString("AAAAA") << QString("AAAAA"));
    QTest::newRow("multibyte string")
        << QString::fromUtf8("\341\233\222\341\233\226\341\232\251\341\232\271\341\232\242\341\233\232\341\232\240\n")
        << (QStringList() << QString::fromUtf8("\341\233\222\341\233\226\341\232\251\341\232\271\341\232\242")
            << QString::fromUtf8("\341\233\232\341\232\240"));
}

// ------------------------------------------------------------------------------
void tst_QTextStream::readLineMaxlen()
{
    QFETCH(QString, input);
    QFETCH(QStringList, lines);
    for (int i = 0; i < 2; ++i) {
        bool useDevice = (i == 1);
        QTextStream stream;
        QFile::remove("testfile");
        QFile file("testfile");
        if (useDevice) {
            QVERIFY(file.open(QIODevice::ReadWrite));
            file.write(input.toUtf8());
            file.seek(0);
            stream.setDevice(&file);
            stream.setEncoding(QStringConverter::Utf8);
        } else {
            stream.setString(&input);
        }

        QStringList list;
        list << stream.readLine(5);
        list << stream.readLine(5);

        QCOMPARE(list, lines);
    }
}

// ------------------------------------------------------------------------------
void tst_QTextStream::readLinesFromBufferCRCR()
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QByteArray data("0123456789\r\r\n");

    for (int i = 0; i < 10000; ++i)
        buffer.write(data);

    buffer.close();
    if (buffer.open(QIODevice::ReadOnly|QIODevice::Text)) {
        QTextStream stream(&buffer);
        while (!stream.atEnd())
            QCOMPARE(stream.readLine(), QString("0123456789"));
    }
}

class ErrorDevice : public QIODevice
{
protected:
    qint64 readData(char *data, qint64 maxlen) override
    {
        Q_UNUSED(data);
        Q_UNUSED(maxlen);
        return -1;
    }

    qint64 writeData(const char *data, qint64 len) override
    {
        Q_UNUSED(data);
        Q_UNUSED(len);
        return -1;
    }
};

void tst_QTextStream::readLineInto()
{
    QByteArray data = "1\n2\n3";

    QTextStream ts(&data);
    QString line;

    ts.readLineInto(&line);
    QCOMPARE(line, QStringLiteral("1"));

    ts.readLineInto(nullptr, 0); // read the second line, but don't store it

    ts.readLineInto(&line);
    QCOMPARE(line, QStringLiteral("3"));

    QVERIFY(!ts.readLineInto(&line));
    QVERIFY(line.isEmpty());

    QFile file(m_rfc3261FilePath);
    QVERIFY(file.open(QFile::ReadOnly));

    ts.setDevice(&file);
    line.reserve(1);
    int maxLineCapacity = line.capacity();

    while (ts.readLineInto(&line)) {
        QVERIFY(line.capacity() >= maxLineCapacity);
        maxLineCapacity = line.capacity();
    }

    line = "Test string";
    ErrorDevice errorDevice;
    QVERIFY(errorDevice.open(QIODevice::ReadOnly));
    ts.setDevice(&errorDevice);

    QVERIFY(!ts.readLineInto(&line));
    QVERIFY(line.isEmpty());
}

// ------------------------------------------------------------------------------
void tst_QTextStream::readLineFromString()
{
    QFETCH(QByteArray, data);
    QFETCH(QStringList, lines);

    QString dataString = data;

    QTextStream stream(&dataString, QIODevice::ReadOnly);
    QStringList list;
    while (!stream.atEnd())
        list << stream.readLine();

    QCOMPARE(list, lines);
}

// ------------------------------------------------------------------------------
void tst_QTextStream::readLineFromStringThenChangeString()
{
    QString first = "First string";
    QString second = "Second string";

    QTextStream stream(&first, QIODevice::ReadOnly);
    QString result = stream.readLine();
    QCOMPARE(first, result);

    stream.setString(&second, QIODevice::ReadOnly);
    result = stream.readLine();
    QCOMPARE(second, result);
}

// ------------------------------------------------------------------------------
void tst_QTextStream::setDevice()
{
    // Check that the read buffer is reset after setting a new device
    QByteArray data1("Hello World");
    QByteArray data2("How are you");

    QBuffer bufferOld(&data1);
    bufferOld.open(QIODevice::ReadOnly);

    QBuffer bufferNew(&data2);
    bufferNew.open(QIODevice::ReadOnly);

    QString text;
    QTextStream stream(&bufferOld);
    QVERIFY(stream >> text);
    QCOMPARE(text, QString("Hello"));

    stream.setDevice(&bufferNew);
    QVERIFY(stream >> text);
    QCOMPARE(text, QString("How"));
}

// ------------------------------------------------------------------------------
void tst_QTextStream::readLineFromTextDevice()
{
    QFETCH(QByteArray, data);
    QFETCH(QStringList, lines);

    for (int i = 0; i < 8; ++i) {
        QBuffer buffer(&data);
        if (i < 4)
            QVERIFY(buffer.open(QIODevice::ReadOnly | QIODevice::Text));
        else
            QVERIFY(buffer.open(QIODevice::ReadOnly));

        QTextStream stream(&buffer);
        QStringList list;
        while (!stream.atEnd()) {
            stream.pos(); // <- triggers side effects
            QString line;

            if (i & 1) {
                QChar c;
                while (!stream.atEnd()) {
                    if (stream >> c) {
                        if (c != QLatin1Char('\n') && c != QLatin1Char('\r'))
                            line += c;
                        if (c == QLatin1Char('\n'))
                            break;
                    }
                }
            } else {
                line = stream.readLine();
            }

            if ((i & 3) == 3 && !QString(QTest::currentDataTag()).contains("utf16"))
                stream.seek(stream.pos());
            list << line;
        }
        QCOMPARE(list, lines);
    }
}

// ------------------------------------------------------------------------------
void tst_QTextStream::generateAllData(bool for_QString) const
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QString>("output");

    // latin-1
    QTest::newRow("empty") << QByteArray() << QString();
    QTest::newRow("latin1-a") << QByteArray("a") << QString("a");
    QTest::newRow("latin1-a\\r") << QByteArray("a\r") << QString("a\r");
    QTest::newRow("latin1-a\\r\\n") << QByteArray("a\r\n") << QString("a\r\n");
    QTest::newRow("latin1-a\\n") << QByteArray("a\n") << QString("a\n");

    // utf-16
    if (!for_QString) {
        // one line
        QTest::newRow("utf16-BE/nothing")
            << QByteArray("\xfe\xff"
                          "\x00\xe5\x00\x67\x00\x65", 8) << QString::fromLatin1("\345ge");
        QTest::newRow("utf16-LE/nothing")
            << QByteArray("\xff\xfe"
                          "\xe5\x00\x67\x00\x65\x00", 8) << QString::fromLatin1("\345ge");
        QTest::newRow("utf16-BE/lf")
            << QByteArray("\xfe\xff"
                          "\x00\xe5\x00\x67\x00\x65\x00\x0a", 10) << QString::fromLatin1("\345ge\n");
        QTest::newRow("utf16-LE/lf")
            << QByteArray("\xff\xfe"
                          "\xe5\x00\x67\x00\x65\x00\x0a\x00", 10) << QString::fromLatin1("\345ge\n");
        QTest::newRow("utf16-BE/crlf")
            << QByteArray("\xfe\xff"
                          "\x00\xe5\x00\x67\x00\x65\x00\x0d\x00\x0a", 12) << QString::fromLatin1("\345ge\r\n");
        QTest::newRow("utf16-LE/crlf")
            << QByteArray("\xff\xfe"
                          "\xe5\x00\x67\x00\x65\x00\x0d\x00\x0a\x00", 12) << QString::fromLatin1("\345ge\r\n");

        // two lines
        QTest::newRow("utf16-BE/twolines")
            << QByteArray("\xfe\xff"
                          "\x00\xe5\x00\x67\x00\x65\x00\x0a"
                          "\x00\xe5\x00\x67\x00\x65\x00\x0a", 18)
            << QString::fromLatin1("\345ge\n\345ge\n");
        QTest::newRow("utf16-LE/twolines")
            << QByteArray("\xff\xfe"
                          "\xe5\x00\x67\x00\x65\x00\x0a\x00"
                          "\xe5\x00\x67\x00\x65\x00\x0a\x00", 18)
            << QString::fromLatin1("\345ge\n\345ge\n");

        // three lines
        QTest::newRow("utf16-BE/threelines")
            << QByteArray("\xfe\xff"
                          "\x00\xe5\x00\x67\x00\x65\x00\x0a"
                          "\x00\xe5\x00\x67\x00\x65\x00\x0a"
                          "\x00\xe5\x00\x67\x00\x65\x00\x0a", 26)
            << QString::fromLatin1("\345ge\n\345ge\n\345ge\n");
        QTest::newRow("utf16-LE/threelines")
            << QByteArray("\xff\xfe"
                          "\xe5\x00\x67\x00\x65\x00\x0a\x00"
                          "\xe5\x00\x67\x00\x65\x00\x0a\x00"
                          "\xe5\x00\x67\x00\x65\x00\x0a\x00", 26)
            << QString::fromLatin1("\345ge\n\345ge\n\345ge\n");
    }
}

// ------------------------------------------------------------------------------
void tst_QTextStream::readLineUntilNull()
{
    QFile file(m_rfc3261FilePath);
    QVERIFY(file.open(QFile::ReadOnly));

    QTextStream stream(&file);
    for (int i = 0; i < 15066; ++i) {
        QString line = stream.readLine();
        QVERIFY(!line.isNull());
        QVERIFY(!line.isNull());
    }
    QVERIFY(!stream.readLine().isNull());
    QVERIFY(stream.readLine().isNull());
}

// ------------------------------------------------------------------------------
void tst_QTextStream::readAllFromDevice()
{
    QFETCH(QByteArray, input);
    QFETCH(QString, output);

    QBuffer buffer(&input);
    buffer.open(QBuffer::ReadOnly);

    QTextStream stream(&buffer);
    QCOMPARE(stream.readAll(), output);
}

// ------------------------------------------------------------------------------
void tst_QTextStream::readAllFromString()
{
    QFETCH(QByteArray, input);
    QFETCH(QString, output);

    QString str = input;

    QTextStream stream(&str);
    QCOMPARE(stream.readAll(), output);
}

// ------------------------------------------------------------------------------
void tst_QTextStream::skipWhiteSpace_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QChar>("output");

    // latin1
    QTest::newRow("empty") << QByteArray() << QChar('\0');
    QTest::newRow(" one") << QByteArray(" one") << QChar('o');
    QTest::newRow("\\none") << QByteArray("\none") << QChar('o');
    QTest::newRow("\\n one") << QByteArray("\n one") << QChar('o');
    QTest::newRow(" \\r\\n one") << QByteArray(" \r\n one") << QChar('o');

    // utf-16
    QTest::newRow("utf16-BE (empty)") << QByteArray("\xfe\xff", 2) << QChar('\0');
    QTest::newRow("utf16-BE ( one)") << QByteArray("\xfe\xff\x00 \x00o\x00n\x00e", 10) << QChar('o');
    QTest::newRow("utf16-BE (\\none)") << QByteArray("\xfe\xff\x00\n\x00o\x00n\x00e", 10) << QChar('o');
    QTest::newRow("utf16-BE (\\n one)") << QByteArray("\xfe\xff\x00\n\x00 \x00o\x00n\x00e", 12) << QChar('o');
    QTest::newRow("utf16-BE ( \\r\\n one)") << QByteArray("\xfe\xff\x00 \x00\r\x00\n\x00 \x00o\x00n\x00e", 16) << QChar('o');

    QTest::newRow("utf16-LE (empty)") << QByteArray("\xff\xfe", 2) << QChar('\0');
    QTest::newRow("utf16-LE ( one)") << QByteArray("\xff\xfe \x00o\x00n\x00e\x00", 10) << QChar('o');
    QTest::newRow("utf16-LE (\\none)") << QByteArray("\xff\xfe\n\x00o\x00n\x00e\x00", 10) << QChar('o');
    QTest::newRow("utf16-LE (\\n one)") << QByteArray("\xff\xfe\n\x00 \x00o\x00n\x00e\x00", 12) << QChar('o');
    QTest::newRow("utf16-LE ( \\r\\n one)") << QByteArray("\xff\xfe \x00\r\x00\n\x00 \x00o\x00n\x00e\x00", 16) << QChar('o');
}

// ------------------------------------------------------------------------------
void tst_QTextStream::skipWhiteSpace()
{
    QFETCH(QByteArray, input);
    QFETCH(QChar, output);

    QBuffer buffer(&input);
    buffer.open(QBuffer::ReadOnly);

    QTextStream stream(&buffer);
    stream.skipWhiteSpace();

    QChar tmp;
    stream >> tmp;

    QCOMPARE(tmp, output);

    QString str = input;
    QTextStream stream2(&input);
    stream2.skipWhiteSpace();

    stream2 >> tmp;

    QCOMPARE(tmp, output);
}

// ------------------------------------------------------------------------------
void tst_QTextStream::lineCount_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<int>("lineCount");

    QTest::newRow("empty") << QByteArray() << 0;
    QTest::newRow("oneline") << QByteArray("a\n") << 1;
    QTest::newRow("twolines") << QByteArray("a\nb\n") << 2;
    QTest::newRow("oneemptyline") << QByteArray("\n") << 1;
    QTest::newRow("twoemptylines") << QByteArray("\n\n") << 2;
    QTest::newRow("buffersize-1 line") << QByteArray(16382, '\n') << 16382;
    QTest::newRow("buffersize line") << QByteArray(16383, '\n') << 16383;
    QTest::newRow("buffersize+1 line") << QByteArray(16384, '\n') << 16384;
    QTest::newRow("buffersize+2 line") << QByteArray(16385, '\n') << 16385;

    QFile file(m_rfc3261FilePath);
    QVERIFY(file.open(QFile::ReadOnly));
    QTest::newRow("rfc3261") << file.readAll() << 15067;
}

// ------------------------------------------------------------------------------
void tst_QTextStream::lineCount()
{
    QFETCH(QByteArray, data);
    QFETCH(int, lineCount);

    QFile out("out.txt");
    QVERIFY(out.open(QFile::WriteOnly));

    QTextStream lineReader(data);
    int lines = 0;
    while (!lineReader.atEnd()) {
        QString line = lineReader.readLine();
        out.write(line.toLatin1() + "\n");
        ++lines;
    }

    out.close();
    QCOMPARE(lines, lineCount);
}

// ------------------------------------------------------------------------------
struct CompareIndicesForArray
{
    int *array;
    CompareIndicesForArray(int *array) : array(array) {}
    bool operator() (const int i1, const int i2)
    {
        return array[i1] < array[i2];
    }
};

void tst_QTextStream::performance()
{
    // Phase #1 - test speed of reading a huge text file with QFile.
    QElapsedTimer stopWatch;

    const int N = 3;
    const char * readMethods[N] = {
        "QFile::readLine()",
        "QTextStream::readLine()",
        "QTextStream::readLine(QString *)"
    };
    int elapsed[N] = {0, 0, 0};

        stopWatch.start();
        int nlines1 = 0;
        QFile file(m_rfc3261FilePath);
        QVERIFY(file.open(QFile::ReadOnly));

        while (!file.atEnd()) {
            ++nlines1;
            file.readLine();
        }

        elapsed[0] = stopWatch.restart();

        int nlines2 = 0;
        QFile file2(m_rfc3261FilePath);
        QVERIFY(file2.open(QFile::ReadOnly));

        QTextStream stream(&file2);
        while (!stream.atEnd()) {
            ++nlines2;
            stream.readLine();
        }

        elapsed[1] = stopWatch.restart();

        int nlines3 = 0;
        QFile file3(m_rfc3261FilePath);
        QVERIFY(file3.open(QFile::ReadOnly));

        QTextStream stream2(&file3);
        QString line;
        while (stream2.readLineInto(&line))
            ++nlines3;

        elapsed[2] = stopWatch.restart();

        QCOMPARE(nlines1, nlines2);
        QCOMPARE(nlines2, nlines3);

    for (int i = 0; i < N; i++) {
        qDebug("%s used %.3f seconds to read the file", readMethods[i],
               elapsed[i] / 1000.0);
    }

    int idx[N] = {0, 1, 2};
    std::sort(idx, idx + N, CompareIndicesForArray(elapsed));

    for (int i = 0; i < N-1; i++) {
        int i1 = idx[i];
        int i2 = idx[i+1];
        qDebug("Reading by %s is %.2fx faster than by %s",
               readMethods[i1],
               double(elapsed[i2]) / double(elapsed[i1]),
               readMethods[i2]);
    }
}

// ------------------------------------------------------------------------------
void tst_QTextStream::hexTest_data()
{
    QTest::addColumn<qlonglong>("number");
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("0") << Q_INT64_C(0) << QByteArray("0x0");
    QTest::newRow("1") << Q_INT64_C(1) << QByteArray("0x1");
    QTest::newRow("2") << Q_INT64_C(2) << QByteArray("0x2");
    QTest::newRow("3") << Q_INT64_C(3) << QByteArray("0x3");
    QTest::newRow("4") << Q_INT64_C(4) << QByteArray("0x4");
    QTest::newRow("5") << Q_INT64_C(5) << QByteArray("0x5");
    QTest::newRow("6") << Q_INT64_C(6) << QByteArray("0x6");
    QTest::newRow("7") << Q_INT64_C(7) << QByteArray("0x7");
    QTest::newRow("8") << Q_INT64_C(8) << QByteArray("0x8");
    QTest::newRow("9") << Q_INT64_C(9) << QByteArray("0x9");
    QTest::newRow("a") << Q_INT64_C(0xa) << QByteArray("0xa");
    QTest::newRow("b") << Q_INT64_C(0xb) << QByteArray("0xb");
    QTest::newRow("c") << Q_INT64_C(0xc) << QByteArray("0xc");
    QTest::newRow("d") << Q_INT64_C(0xd) << QByteArray("0xd");
    QTest::newRow("e") << Q_INT64_C(0xe) << QByteArray("0xe");
    QTest::newRow("f") << Q_INT64_C(0xf) << QByteArray("0xf");
    QTest::newRow("-1") << Q_INT64_C(-1) << QByteArray("-0x1");
    QTest::newRow("0xffffffff") << Q_INT64_C(0xffffffff) << QByteArray("0xffffffff");
    QTest::newRow("0xfffffffffffffffe") << Q_INT64_C(0xfffffffffffffffe) << QByteArray("-0x2");
    QTest::newRow("0xffffffffffffffff") << Q_INT64_C(0xffffffffffffffff) << QByteArray("-0x1");
    QTest::newRow("0x7fffffffffffffff") << Q_INT64_C(0x7fffffffffffffff) << QByteArray("0x7fffffffffffffff");
}

// ------------------------------------------------------------------------------
void tst_QTextStream::hexTest()
{
    QFETCH(qlonglong, number);
    QFETCH(QByteArray, data);

    QByteArray array;
    QTextStream stream(&array);

    stream << Qt::showbase << Qt::hex << number;
    stream.flush();
    QCOMPARE(array, data);
}

// ------------------------------------------------------------------------------
void tst_QTextStream::binTest_data()
{
    QTest::addColumn<int>("number");
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("0") << 0 << QByteArray("0b0");
    QTest::newRow("1") << 1 << QByteArray("0b1");
    QTest::newRow("2") << 2 << QByteArray("0b10");
    QTest::newRow("5") << 5 << QByteArray("0b101");
    QTest::newRow("-1") << -1 << QByteArray("-0b1");
    QTest::newRow("11111111") << 0xff << QByteArray("0b11111111");
    QTest::newRow("1111111111111111") << 0xffff << QByteArray("0b1111111111111111");
    QTest::newRow("1111111011111110") << 0xfefe << QByteArray("0b1111111011111110");
}

// ------------------------------------------------------------------------------
void tst_QTextStream::binTest()
{
    QFETCH(int, number);
    QFETCH(QByteArray, data);

    QByteArray array;
    QTextStream stream(&array);

    QVERIFY(stream << Qt::showbase << Qt::bin << number);
    stream.flush();
    QCOMPARE(array.constData(), data.constData());
}

// ------------------------------------------------------------------------------
void tst_QTextStream::octTest_data()
{
    QTest::addColumn<int>("number");
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("0") << 0 << QByteArray("00");
    QTest::newRow("40") << 40 << QByteArray("050");
}

// ------------------------------------------------------------------------------
void tst_QTextStream::octTest()
{
    QFETCH(int, number);
    QFETCH(QByteArray, data);

    QByteArray array;
    QTextStream stream(&array);

    QVERIFY(stream << Qt::showbase << Qt::oct << number);
    stream.flush();
    QCOMPARE(array, data);
}

// ------------------------------------------------------------------------------
void tst_QTextStream::zeroTermination()
{
    QTextStream stream;
    char c = '@';

    QTest::ignoreMessage(QtWarningMsg, "QTextStream: No device");
    QVERIFY(stream >> c);
    QCOMPARE(c, '\0');

    c = '@';

    QTest::ignoreMessage(QtWarningMsg, "QTextStream: No device");
    QVERIFY(stream >> &c);
    QCOMPARE(c, '\0');
}

// ------------------------------------------------------------------------------
void tst_QTextStream::ws_manipulator()
{
    {
        QString string = "a b c d";
        QTextStream stream(&string);

        char a, b, c, d;
        QVERIFY(stream >> a >> b >> c >> d);
        QCOMPARE(a, 'a');
        QCOMPARE(b, ' ');
        QCOMPARE(c, 'b');
        QCOMPARE(d, ' ');
    }
    {
        QString string = "a b c d";
        QTextStream stream(&string);

        char a, b, c, d;
        QVERIFY(stream >> a >> Qt::ws >> b >> Qt::ws >> c >> Qt::ws >> d);
        QCOMPARE(a, 'a');
        QCOMPARE(b, 'b');
        QCOMPARE(c, 'c');
        QCOMPARE(d, 'd');
    }
}

// ------------------------------------------------------------------------------
void tst_QTextStream::stillOpenWhenAtEnd()
{
    QFile file(QFINDTESTDATA("tst_qtextstream.cpp"));
    QVERIFY(file.open(QFile::ReadOnly));

    QTextStream stream(&file);
    while (!stream.readLine().isNull()) {}
    QVERIFY(file.isOpen());

#ifdef QT_TEST_SERVER
    if (!QtNetworkSettings::verifyConnection(QtNetworkSettings::imapServerName(), 143))
        QSKIP("No network test server available");
#else
    if (!QtNetworkSettings::verifyTestNetworkSettings())
        QSKIP("No network test server available");
#endif

    QTcpSocket socket;
    socket.connectToHost(QtNetworkSettings::imapServerName(), 143);
    QTRY_VERIFY_WITH_TIMEOUT(socket.bytesAvailable() > 0, 20000);

    QTextStream stream2(&socket);
    while (!stream2.readLine().isNull()) {}
    QVERIFY(socket.isOpen());
}

// ------------------------------------------------------------------------------
void tst_QTextStream::readNewlines_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QString>("output");

    QTest::newRow("empty") << QByteArray() << QString();
    QTest::newRow("\\r\\n") << QByteArray("\r\n") << QString("\n");
    QTest::newRow("\\r\\r\\n") << QByteArray("\r\r\n") << QString("\n");
    QTest::newRow("\\r\\n\\r\\n") << QByteArray("\r\n\r\n") << QString("\n\n");
    QTest::newRow("\\n") << QByteArray("\n") << QString("\n");
    QTest::newRow("\\n\\n") << QByteArray("\n\n") << QString("\n\n");
}

// ------------------------------------------------------------------------------
void tst_QTextStream::readNewlines()
{
    QFETCH(QByteArray, input);
    QFETCH(QString, output);

    QBuffer buffer(&input);
    buffer.open(QBuffer::ReadOnly | QBuffer::Text);
    QTextStream stream(&buffer);
    QCOMPARE(stream.readAll(), output);
}

// ------------------------------------------------------------------------------
void tst_QTextStream::seek()
{
    QFile file(m_rfc3261FilePath);
    QVERIFY(file.open(QFile::ReadOnly));

    QTextStream stream(&file);
    QString tmp;
    QVERIFY(stream >> tmp);
    QCOMPARE(tmp, QString::fromLatin1("Network"));

    // QTextStream::seek(0) should both clear its internal read/write buffers
    // and seek the device.
    for (int i = 0; i < 4; ++i) {
        stream.seek(12 + i);
        QVERIFY(stream >> tmp);
        QCOMPARE(tmp, QString("Network").mid(i));
    }
    for (int i = 0; i < 4; ++i) {
        stream.seek(16 - i);
        QVERIFY(stream >> tmp);
        QCOMPARE(tmp, QString("Network").mid(4 - i));
    }
    stream.seek(139181);
    QVERIFY(stream >> tmp);
    QCOMPARE(tmp, QString("information"));
    stream.seek(388683);
    QVERIFY(stream >> tmp);
    QCOMPARE(tmp, QString("telephone"));

    // Also test this with a string
    QString words = QLatin1String("thisisa");
    QTextStream stream2(&words, QIODevice::ReadOnly);
    QVERIFY(stream2 >> tmp);
    QCOMPARE(tmp, QString::fromLatin1("thisisa"));

    for (int i = 0; i < 4; ++i) {
        stream2.seek(i);
        QVERIFY(stream2 >> tmp);
        QCOMPARE(tmp, QString("thisisa").mid(i));
    }
    for (int i = 0; i < 4; ++i) {
        stream2.seek(4 - i);
        QVERIFY(stream2 >> tmp);
        QCOMPARE(tmp, QString("thisisa").mid(4 - i));
    }
}

// ------------------------------------------------------------------------------
void tst_QTextStream::pos()
{
    {
        // Strings
        QString str("this is a test");
        QTextStream stream(&str, QIODevice::ReadWrite);

        QCOMPARE(stream.pos(), qint64(0));
        for (int i = 0; i <= str.size(); ++i) {
            QVERIFY(stream.seek(i));
            QCOMPARE(stream.pos(), qint64(i));
        }
        for (int j = str.size(); j >= 0; --j) {
            QVERIFY(stream.seek(j));
            QCOMPARE(stream.pos(), qint64(j));
        }

        QVERIFY(stream.seek(0));

        QChar ch;
        QVERIFY(stream >> ch);
        QCOMPARE(ch, QChar('t'));

        QCOMPARE(stream.pos(), qint64(1));
        QVERIFY(stream.seek(1));
        QCOMPARE(stream.pos(), qint64(1));
        QVERIFY(stream.seek(0));

        QString strtmp;
        QVERIFY(stream >> strtmp);
        QCOMPARE(strtmp, QString("this"));

        QCOMPARE(stream.pos(), qint64(4));
        stream.seek(0);
        stream.seek(4);

        QVERIFY(stream >> ch);
        QCOMPARE(ch, QChar(' '));
        QCOMPARE(stream.pos(), qint64(5));

        stream.seek(10);
        QVERIFY(stream >> strtmp);
        QCOMPARE(strtmp, QString("test"));
        QCOMPARE(stream.pos(), qint64(14));
    }
    {
        // Latin1 device
        QFile file(m_rfc3261FilePath);
        QVERIFY(file.open(QIODevice::ReadOnly));

        QTextStream stream(&file);

        QCOMPARE(stream.pos(), qint64(0));

        for (int i = 0; i <= file.size(); i += 7) {
            QVERIFY(stream.seek(i));
            QCOMPARE(stream.pos(), qint64(i));
        }
        for (int j = file.size(); j >= 0; j -= 7) {
            QVERIFY(stream.seek(j));
            QCOMPARE(stream.pos(), qint64(j));
        }

        stream.seek(0);

        QString strtmp;
        QVERIFY(stream >> strtmp);
        QCOMPARE(strtmp, QString("Network"));
        QCOMPARE(stream.pos(), qint64(19));

        stream.seek(2598);
        QCOMPARE(stream.pos(), qint64(2598));
        QVERIFY(stream >> strtmp);
        QCOMPARE(stream.pos(), qint64(2607));
        QCOMPARE(strtmp, QString("locations"));
    }
}

// ------------------------------------------------------------------------------
void tst_QTextStream::pos2()
{
    QByteArray data("abcdef\r\nghijkl\r\n");
    QBuffer buffer(&data);
    QVERIFY(buffer.open(QIODevice::ReadOnly | QIODevice::Text));

    QTextStream stream(&buffer);

    QChar ch;

    QCOMPARE(stream.pos(), qint64(0));
    QVERIFY(stream >> ch);
    QCOMPARE(ch, QChar('a'));
    QCOMPARE(stream.pos(), qint64(1));

    QString str;
    QVERIFY(stream >> str);
    QCOMPARE(str, QString("bcdef"));
    QCOMPARE(stream.pos(), qint64(6));

    QVERIFY(stream >> str);
    QCOMPARE(str, QString("ghijkl"));
    QCOMPARE(stream.pos(), qint64(14));

    // Seek back and try again
    stream.seek(1);
    QCOMPARE(stream.pos(), qint64(1));
    QVERIFY(stream >> str);
    QCOMPARE(str, QString("bcdef"));
    QCOMPARE(stream.pos(), qint64(6));

    stream.seek(6);
    QVERIFY(stream >> str);
    QCOMPARE(str, QString("ghijkl"));
    QCOMPARE(stream.pos(), qint64(14));
}

// ------------------------------------------------------------------------------
void tst_QTextStream::pos3LargeFile()
{
    // NOTE: The unusual spacing is to ensure non-1-character whitespace.
    constexpr auto lineString = " 0  1  2\t3  4\t \t5  6  7  8   9 \n"_L1;
    {
        QFile file(testFileName);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out( &file );
        // Approximately 5kb text file (more is too slow (QTBUG-138435))
        const int NbLines = (5 * 1024) / lineString.size() + 1;
        for (int line = 0; line < NbLines; ++line)
            QVERIFY(out << lineString);
        // File is automatically flushed and closed on destruction.
    }
    QFile file(testFileName);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream in( &file );
    constexpr int testValues[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    qint64 expectedLineEnd = 0;
#ifdef Q_OS_WIN // CRLF platform
    constexpr int crlfAdjustment = 1;
#else
    constexpr int crlfAdjustment = 0;
#endif
    const auto expectedLineLength = lineString.size() + crlfAdjustment;
    QCOMPARE(in.pos(), 0);
    while (true) {
        for (size_t i = 0; i < std::size(testValues); ++i) {
            int value = -42;
            if (!(in >> value)) {
                // End case, i == 0 && eof reached.
                QCOMPARE(i, 0);
                QCOMPARE(in.status(), QTextStream::ReadPastEnd);
                return;
            }
            QCOMPARE(value, testValues[i]);
        }
        expectedLineEnd += expectedLineLength;
        // Final space and newline are not consumed until next read.
        QCOMPARE(in.pos(), expectedLineEnd - 2 - crlfAdjustment);
    }
}

// ------------------------------------------------------------------------------
void tst_QTextStream::readStdin()
{
#if !QT_CONFIG(process)
    QSKIP("No qprocess support");
#else
    QProcess stdinProcess;
    stdinProcess.start("stdinProcess/stdinProcess");
    stdinProcess.setReadChannel(QProcess::StandardError);

    QTextStream stream(&stdinProcess);
    QVERIFY(stream << "1" << Qt::endl);
    QVERIFY(stream << "2" << Qt::endl);
    QVERIFY(stream << "3" << Qt::endl);

    stdinProcess.closeWriteChannel();

    QVERIFY(stdinProcess.waitForFinished(5000));

    int a, b, c;
    QVERIFY(stream >> a >> b >> c);
    QCOMPARE(a, 1);
    QCOMPARE(b, 2);
    QCOMPARE(c, 3);
#endif
}

// ------------------------------------------------------------------------------
void tst_QTextStream::readAllFromStdin()
{
#if !QT_CONFIG(process)
    QSKIP("No qprocess support");
#else
    QProcess stdinProcess;
    stdinProcess.start("readAllStdinProcess/readAllStdinProcess", {}, QIODevice::ReadWrite | QIODevice::Text);
    stdinProcess.setReadChannel(QProcess::StandardError);

    QTextStream stream(&stdinProcess);
    stream.setEncoding(QStringConverter::Latin1);
    QVERIFY(stream << "hello world" << Qt::flush);

    stdinProcess.closeWriteChannel();

    QVERIFY(stdinProcess.waitForFinished(5000));
    QCOMPARE(stream.readAll(), QString::fromLatin1("hello world\n"));
#endif
}

// ------------------------------------------------------------------------------
void tst_QTextStream::readLineFromStdin()
{
#if !QT_CONFIG(process)
    QSKIP("No qprocess support");
#else
    QProcess stdinProcess;
    stdinProcess.start("readLineStdinProcess/readLineStdinProcess", {}, QIODevice::ReadWrite | QIODevice::Text);
    stdinProcess.setReadChannel(QProcess::StandardError);

    stdinProcess.write("abc\n");
    QVERIFY(stdinProcess.waitForReadyRead());
    QCOMPARE(stdinProcess.readAll(), "abc");

    stdinProcess.write("def\n");
    QVERIFY(stdinProcess.waitForReadyRead());
    QCOMPARE(stdinProcess.readAll(), "def");

    stdinProcess.closeWriteChannel();

    QVERIFY(stdinProcess.waitForFinished(5000));
#endif
}

// ------------------------------------------------------------------------------
void tst_QTextStream::read()
{
    {
        QFile::remove("testfile");
        QFile file("testfile");
        QVERIFY(file.open(QFile::WriteOnly));
        file.write("4.15 abc ole");
        file.close();

        QVERIFY(file.open(QFile::ReadOnly));
        QTextStream stream(&file);
        QCOMPARE(stream.read(0), QString(""));
        QCOMPARE(stream.read(4), QString("4.15"));
        QCOMPARE(stream.read(4), QString(" abc"));
        stream.seek(1);
        QCOMPARE(stream.read(4), QString(".15 "));
        stream.seek(1);
        QCOMPARE(stream.read(4), QString(".15 "));
        stream.seek(2);
        QCOMPARE(stream.read(4), QString("15 a"));
        // ### add tests for reading \r\n etc..
    }

    {
        // File larger than QTEXTSTREAM_BUFFERSIZE
        QFile::remove("testfile");
        QFile file("testfile");
        QVERIFY(file.open(QFile::WriteOnly));
        for (int i = 0; i < 16384 / 8; ++i)
            file.write("01234567");
        file.write("0");
        file.close();

        QVERIFY(file.open(QFile::ReadOnly));
        QTextStream stream(&file);
        QCOMPARE(stream.read(10), QString("0123456701"));
        QCOMPARE(stream.read(10), QString("2345670123"));
        QCOMPARE(stream.readAll().size(), 16385-20);
    }
}

// ------------------------------------------------------------------------------
void tst_QTextStream::qbool()
{
    QString s;
    QTextStream stream(&s);
    QVERIFY(stream << s.contains(QString("hei")));
    QCOMPARE(s, QString("0"));
}

// ------------------------------------------------------------------------------
void tst_QTextStream::forcePoint()
{
    QString str;
    QTextStream stream(&str);
    QVERIFY(stream << Qt::fixed << Qt::forcepoint << 1.0 << ' ' << 1 << ' ' << 0
                   << ' ' << -1.0 << ' ' << -1);
    QCOMPARE(str, QString("1.000000 1 0 -1.000000 -1"));

    str.clear();
    stream.seek(0);
    QVERIFY(stream << Qt::scientific << Qt::forcepoint << 1.0 << ' ' << 1
                   << ' ' << 0 << ' ' << -1.0 << ' ' << -1);
    QCOMPARE(str, QString("1.000000e+00 1 0 -1.000000e+00 -1"));

    str.clear();
    stream.seek(0);
    stream.setRealNumberNotation(QTextStream::SmartNotation);
    QVERIFY(stream << Qt::forcepoint << 1.0 << ' ' << 1 << ' ' << 0 << ' ' << -1.0 << ' ' << -1);
    QCOMPARE(str, QString("1.00000 1 0 -1.00000 -1"));

}

// ------------------------------------------------------------------------------
void tst_QTextStream::forceSign()
{
    QString str;
    QTextStream stream(&str);
    QVERIFY(stream << Qt::forcesign << 1.2 << ' ' << -1.2 << ' ' << 0);
    QCOMPARE(str, QString("+1.2 -1.2 +0"));
}

// ------------------------------------------------------------------------------
void tst_QTextStream::read0d0d0a()
{
    QFile file(QFINDTESTDATA("task113817.txt"));
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));

    QTextStream stream(&file);
    while (!stream.atEnd())
        stream.readLine();
}

// ------------------------------------------------------------------------------

Q_DECLARE_METATYPE(QTextStreamFunction);

// Also tests that we can have namespaces that conflict with our QTextStream constants.
namespace ws {
QTextStream &noop(QTextStream &s) { return s; }
}

void tst_QTextStream::numeralCase_data()
{
    QTextStreamFunction noop_ = ws::noop;
    QTextStreamFunction bin  = Qt::bin;
    QTextStreamFunction oct  = Qt::oct;
    QTextStreamFunction hex  = Qt::hex;
    QTextStreamFunction base  = Qt::showbase;
    QTextStreamFunction ucb   = Qt::uppercasebase;
    QTextStreamFunction lcb   = Qt::lowercasebase;
    QTextStreamFunction ucd   = Qt::uppercasedigits;
    QTextStreamFunction lcd   = Qt::lowercasedigits;

    QTest::addColumn<QTextStreamFunction>("func1");
    QTest::addColumn<QTextStreamFunction>("func2");
    QTest::addColumn<QTextStreamFunction>("func3");
    QTest::addColumn<QTextStreamFunction>("func4");
    QTest::addColumn<int>("value");
    QTest::addColumn<QString>("expected");
    QTest::newRow("dec 1") << noop_ << noop_ << noop_ << noop_ << 31 << "31";
    QTest::newRow("dec 2") << noop_ << base  << noop_ << noop_ << 31 << "31";

    QTest::newRow("hex 1")  << hex  << noop_ << noop_ << noop_ << 31 << "1f";
    QTest::newRow("hex 2")  << hex  << noop_ << noop_ << lcd   << 31 << "1f";
    QTest::newRow("hex 3")  << hex  << noop_ << ucb   << noop_ << 31 << "1f";
    QTest::newRow("hex 4")  << hex  << noop_ << noop_ << ucd   << 31 << "1F";
    QTest::newRow("hex 5")  << hex  << noop_ << lcb   << ucd   << 31 << "1F";
    QTest::newRow("hex 6")  << hex  << noop_ << ucb   << ucd   << 31 << "1F";
    QTest::newRow("hex 7")  << hex  << base  << noop_ << noop_ << 31 << "0x1f";
    QTest::newRow("hex 8")  << hex  << base  << lcb   << lcd   << 31 << "0x1f";
    QTest::newRow("hex 9")  << hex  << base  << ucb   << noop_ << 31 << "0X1f";
    QTest::newRow("hex 10") << hex  << base  << ucb   << lcd   << 31 << "0X1f";
    QTest::newRow("hex 11") << hex  << base  << noop_ << ucd   << 31 << "0x1F";
    QTest::newRow("hex 12") << hex  << base  << lcb   << ucd   << 31 << "0x1F";
    QTest::newRow("hex 13") << hex  << base  << ucb   << ucd   << 31 << "0X1F";

    QTest::newRow("bin 1") << bin  << noop_ << noop_ << noop_ << 31 << "11111";
    QTest::newRow("bin 2") << bin  << base  << noop_ << noop_ << 31 << "0b11111";
    QTest::newRow("bin 3") << bin  << base  << lcb   << noop_ << 31 << "0b11111";
    QTest::newRow("bin 4") << bin  << base  << ucb   << noop_ << 31 << "0B11111";
    QTest::newRow("bin 5") << bin  << base  << noop_ << ucd   << 31 << "0b11111";
    QTest::newRow("bin 6") << bin  << base  << lcb   << ucd   << 31 << "0b11111";
    QTest::newRow("bin 7") << bin  << base  << ucb   << ucd   << 31 << "0B11111";

    QTest::newRow("oct 1") << oct  << noop_ << noop_ << noop_ << 31 << "37";
    QTest::newRow("oct 2") << oct  << base  << noop_ << noop_ << 31 << "037";
}

void tst_QTextStream::numeralCase()
{
    QFETCH(QTextStreamFunction, func1);
    QFETCH(QTextStreamFunction, func2);
    QFETCH(QTextStreamFunction, func3);
    QFETCH(QTextStreamFunction, func4);
    QFETCH(int, value);
    QFETCH(QString, expected);

    QString str;
    QTextStream stream(&str);
    QVERIFY(stream << func1 << func2 << func3 << func4 << value);
    QCOMPARE(str, expected);
}

// ------------------------------------------------------------------------------
void tst_QTextStream::nanInf()
{
    // Cannot use test data in this function, as comparing nans and infs isn't
    // well defined.
    QString str("nan NAN nAn +nan +NAN +nAn -nan -NAN -nAn"
                " inf INF iNf +inf +INF +iNf -inf -INF -iNf");

    QTextStream stream(&str);

    double tmpD = 0;
    QVERIFY(stream >> tmpD); QVERIFY(qIsNaN(tmpD)); tmpD = 0;
    QVERIFY(stream >> tmpD); QVERIFY(qIsNaN(tmpD)); tmpD = 0;
    QVERIFY(stream >> tmpD); QVERIFY(qIsNaN(tmpD)); tmpD = 0;
    QVERIFY(stream >> tmpD); QVERIFY(qIsNaN(tmpD)); tmpD = 0;
    QVERIFY(stream >> tmpD); QVERIFY(qIsNaN(tmpD)); tmpD = 0;
    QVERIFY(stream >> tmpD); QVERIFY(qIsNaN(tmpD)); tmpD = 0;
    QVERIFY(stream >> tmpD); QVERIFY(qIsNaN(tmpD)); tmpD = 0;
    QVERIFY(stream >> tmpD); QVERIFY(qIsNaN(tmpD)); tmpD = 0;
    QVERIFY(stream >> tmpD); QVERIFY(qIsNaN(tmpD)); tmpD = 0;
    QVERIFY(stream >> tmpD); QVERIFY(qIsInf(tmpD)); QVERIFY(tmpD > 0); tmpD = 0;
    QVERIFY(stream >> tmpD); QVERIFY(qIsInf(tmpD)); QVERIFY(tmpD > 0); tmpD = 0;
    QVERIFY(stream >> tmpD); QVERIFY(qIsInf(tmpD)); QVERIFY(tmpD > 0); tmpD = 0;
    QVERIFY(stream >> tmpD); QVERIFY(qIsInf(tmpD)); QVERIFY(tmpD > 0); tmpD = 0;
    QVERIFY(stream >> tmpD); QVERIFY(qIsInf(tmpD)); QVERIFY(tmpD > 0); tmpD = 0;
    QVERIFY(stream >> tmpD); QVERIFY(qIsInf(tmpD)); QVERIFY(tmpD > 0); tmpD = 0;
    QVERIFY(stream >> tmpD); QVERIFY(qIsInf(tmpD)); QVERIFY(tmpD < 0); tmpD = 0;
    QVERIFY(stream >> tmpD); QVERIFY(qIsInf(tmpD)); QVERIFY(tmpD < 0); tmpD = 0;
    QVERIFY(stream >> tmpD); QVERIFY(qIsInf(tmpD)); QVERIFY(tmpD < 0); tmpD = 0;

    stream.seek(0);

    float tmpF = 0;
    QVERIFY(stream >> tmpF); QVERIFY(qIsNaN(tmpF)); tmpD = 0;
    QVERIFY(stream >> tmpF); QVERIFY(qIsNaN(tmpF)); tmpD = 0;
    QVERIFY(stream >> tmpF); QVERIFY(qIsNaN(tmpF)); tmpD = 0;
    QVERIFY(stream >> tmpF); QVERIFY(qIsNaN(tmpF)); tmpD = 0;
    QVERIFY(stream >> tmpF); QVERIFY(qIsNaN(tmpF)); tmpD = 0;
    QVERIFY(stream >> tmpF); QVERIFY(qIsNaN(tmpF)); tmpD = 0;
    QVERIFY(stream >> tmpF); QVERIFY(qIsNaN(tmpF)); tmpD = 0;
    QVERIFY(stream >> tmpF); QVERIFY(qIsNaN(tmpF)); tmpD = 0;
    QVERIFY(stream >> tmpF); QVERIFY(qIsNaN(tmpF)); tmpD = 0;
    QVERIFY(stream >> tmpF); QVERIFY(qIsInf(tmpF)); QVERIFY(tmpF > 0); tmpD = 0;
    QVERIFY(stream >> tmpF); QVERIFY(qIsInf(tmpF)); QVERIFY(tmpF > 0); tmpD = 0;
    QVERIFY(stream >> tmpF); QVERIFY(qIsInf(tmpF)); QVERIFY(tmpF > 0); tmpD = 0;
    QVERIFY(stream >> tmpF); QVERIFY(qIsInf(tmpF)); QVERIFY(tmpF > 0); tmpD = 0;
    QVERIFY(stream >> tmpF); QVERIFY(qIsInf(tmpF)); QVERIFY(tmpF > 0); tmpD = 0;
    QVERIFY(stream >> tmpF); QVERIFY(qIsInf(tmpF)); QVERIFY(tmpF > 0); tmpD = 0;
    QVERIFY(stream >> tmpF); QVERIFY(qIsInf(tmpF)); QVERIFY(tmpF < 0); tmpD = 0;
    QVERIFY(stream >> tmpF); QVERIFY(qIsInf(tmpF)); QVERIFY(tmpF < 0); tmpD = 0;
    QVERIFY(stream >> tmpF); QVERIFY(qIsInf(tmpF)); QVERIFY(tmpF < 0);

    QString s;
    QTextStream out(&s);
    QVERIFY(out << qInf() << ' ' << -qInf() << ' ' << qQNaN()
                << Qt::uppercasedigits << ' '
                << qInf() << ' ' << -qInf() << ' ' << qQNaN()
                << Qt::flush);

    QCOMPARE(s, QString("inf -inf nan INF -INF NAN"));
}

// ------------------------------------------------------------------------------
void tst_QTextStream::utf8IncompleteAtBufferBoundary_data()
{
    QTest::addColumn<bool>("useLocale");

    QTest::newRow("utf8") << false;

    // is this locale UTF-8?
    if (QString(QChar::ReplacementCharacter).toLocal8Bit() == "\xef\xbf\xbd")
        QTest::newRow("locale") << true;
}

void tst_QTextStream::utf8IncompleteAtBufferBoundary()
{
    QFile::remove(testFileName);
    QFile data(testFileName);

    QString lineContents = QString::fromUtf8("\342\200\223" // U+2013 EN DASH
                                             "\342\200\223"
                                             "\342\200\223"
                                             "\342\200\223"
                                             "\342\200\223"
                                             "\342\200\223");

    QVERIFY(data.open(QFile::WriteOnly | QFile::Truncate));
    {
        QTextStream out(&data);
        out.setEncoding(QStringConverter::Utf8);
        out.setFieldWidth(3);

        for (int i = 0; i < 1000; ++i) {
            QVERIFY(out << i << lineContents << Qt::endl);
        }
    }
    data.close();

    QVERIFY(data.open(QFile::ReadOnly));
    QTextStream in(&data);

    QFETCH(bool, useLocale);
    if (!useLocale)
        in.setEncoding(QStringConverter::Utf8);
    else
        in.setEncoding(QStringConverter::System);

    int i = 0;
    do {
        QString line = in.readLine().trimmed();
        ++i;
        QVERIFY2(line.endsWith(lineContents), QString("Line %1: %2").arg(i).arg(line).toLocal8Bit());
    } while (!in.atEnd());
}

// ------------------------------------------------------------------------------

// Make sure we don't write a BOM after seek()ing

void tst_QTextStream::writeSeekWriteNoBOM()
{

    //First with the default codec (normally either latin-1 or UTF-8)

    QBuffer out;
    out.open(QIODevice::WriteOnly);
    QTextStream stream(&out);

    int number = 0;
    QString sizeStr = QLatin1String("Size=")
        + QString::number(number).rightJustified(10, QLatin1Char('0'));
    QVERIFY(stream << sizeStr << Qt::endl);
    QVERIFY(stream << "Version=" << QString::number(14) << Qt::endl);
    QVERIFY(stream << "blah blah blah" << Qt::endl);
    stream.flush();

    QCOMPARE(out.data(), "Size=0000000000\nVersion=14\nblah blah blah\n");

    // Now overwrite the size header item
    number = 42;
    stream.seek(0);
    sizeStr = QLatin1String("Size=")
        + QString::number(number).rightJustified(10, QLatin1Char('0'));
    QVERIFY(stream << sizeStr << Qt::endl);
    stream.flush();

    // Check buffer is still OK
    QCOMPARE(out.data(), "Size=0000000042\nVersion=14\nblah blah blah\n");


    //Then UTF-16

    QBuffer out16;
    out16.open(QIODevice::WriteOnly);
    QTextStream stream16(&out16);
    stream16.setEncoding(QStringConverter::Utf16);

    QVERIFY(stream16 << "one" << "two" << QLatin1String("three"));
    stream16.flush();

    // save that output
    const QByteArray first = out16.data();

    stream16.seek(0);
    QVERIFY(stream16 << "one");
    stream16.flush();

    QCOMPARE(out16.data(), first);
}

// ------------------------------------------------------------------------------
void tst_QTextStream::generateOperatorCharData(bool for_QString) const
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QChar>("qchar_output");
    QTest::addColumn<char>("char_output");
    QTest::addColumn<QByteArray>("write_output");
    QTest::addColumn<bool>("status");

    QTest::newRow("empty") << QByteArray() << QChar('\0') << '\0'
                           << QByteArray("\0", 1) << false;
    QTest::newRow("a") << QByteArray("a") << QChar('a') << 'a'
                       << QByteArray("a") << true;
    QTest::newRow("\\na") << QByteArray("\na") << QChar('\n') << '\n'
                          << QByteArray("\n") << true;
    QTest::newRow("\\0") << QByteArray("\0") << QChar('\0') << '\0'
                         << QByteArray("\0", 1) << false;
    QTest::newRow("\\xff") << QByteArray("\xff") << QChar('\xff') << '\xff'
                           << QByteArray("\xff") << true;
    QTest::newRow("\\xfe") << QByteArray("\xfe") << QChar('\xfe') << '\xfe'
                           << QByteArray("\xfe") << true;

    if (!for_QString) {
        QTest::newRow("utf16-BE (empty)") << QByteArray("\xff\xfe", 2) << QChar('\0') << '\0'
                                          << QByteArray("\0", 1) << false;
        QTest::newRow("utf16-BE (a)") << QByteArray("\xff\xfe\x61\x00", 4) << QChar('a') << 'a'
                                      << QByteArray("a") << true;
        QTest::newRow("utf16-LE (empty)") << QByteArray("\xfe\xff", 2) << QChar('\0') << '\0'
                                          << QByteArray("\0", 1) << false;
        QTest::newRow("utf16-LE (a)") << QByteArray("\xfe\xff\x00\x61", 4) << QChar('a') << 'a'
                                      << QByteArray("a") << true;
    }
}

// ------------------------------------------------------------------------------
void tst_QTextStream::QChar_operators_FromDevice()
{
    QFETCH(QByteArray, input);
    QFETCH(QChar, qchar_output);
    QFETCH(QByteArray, write_output);
    QFETCH(bool, status);

    QBuffer buf(&input);
    buf.open(QBuffer::ReadOnly);
    QTextStream stream(&buf);
    stream.setEncoding(QStringConverter::Latin1);
    QChar tmp;
    QCOMPARE(static_cast<bool>(stream >> tmp), status);
    QCOMPARE(tmp, qchar_output);

    QBuffer writeBuf;
    writeBuf.open(QBuffer::WriteOnly);

    QTextStream writeStream(&writeBuf);
    writeStream.setEncoding(QStringConverter::Latin1);
    QVERIFY(writeStream << qchar_output);
    writeStream.flush();

    QCOMPARE(writeBuf.data(), write_output);
}

// ------------------------------------------------------------------------------
void tst_QTextStream::char16_t_operators_FromDevice()
{
    QFETCH(QByteArray, input);
    QFETCH(const QChar, qchar_output);
    QFETCH(const QByteArray, write_output);
    QFETCH(bool, status);
    const char16_t char16_t_output = qchar_output.unicode();

    QBuffer buf(&input);
    buf.open(QBuffer::ReadOnly);
    QTextStream stream(&buf);
    stream.setEncoding(QStringConverter::Latin1);
    char16_t tmp;
    QCOMPARE(static_cast<bool>(stream >> tmp), status);
    QCOMPARE(tmp, qchar_output);

    QBuffer writeBuf;
    writeBuf.open(QBuffer::WriteOnly);

    QTextStream writeStream(&writeBuf);
    writeStream.setEncoding(QStringConverter::Latin1);
    QVERIFY(writeStream << char16_t_output);
    writeStream.flush();

    QCOMPARE(writeBuf.data(), write_output);
}

// ------------------------------------------------------------------------------
void tst_QTextStream::char_operators_FromDevice()
{
    QFETCH(QByteArray, input);
    QFETCH(char, char_output);
    QFETCH(QByteArray, write_output);
    QFETCH(bool, status);

    QBuffer buf(&input);
    buf.open(QBuffer::ReadOnly);
    QTextStream stream(&buf);
    stream.setEncoding(QStringConverter::Latin1);
    char tmp;
    QCOMPARE(static_cast<bool>(stream >> tmp), status);
    QCOMPARE(tmp, char_output);

    QBuffer writeBuf;
    writeBuf.open(QBuffer::WriteOnly);

    QTextStream writeStream(&writeBuf);
    writeStream.setEncoding(QStringConverter::Latin1);
    QVERIFY(writeStream << char_output);
    writeStream.flush();

    QCOMPARE(writeBuf.data(), write_output);
}

// ------------------------------------------------------------------------------
void tst_QTextStream::generateNaturalNumbersData(bool for_QString) const
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<qulonglong>("output");

    QTest::newRow("empty") << QByteArray() << qulonglong(0);
    QTest::newRow("a") << QByteArray("a") << qulonglong(0);
    QTest::newRow(" ") << QByteArray(" ") << qulonglong(0);
    QTest::newRow("0") << QByteArray("0") << qulonglong(0);
    QTest::newRow("1") << QByteArray("1") << qulonglong(1);
    QTest::newRow("12") << QByteArray("12") << qulonglong(12);
    QTest::newRow("-12") << QByteArray("-12") << qulonglong(-12);
    QTest::newRow("-0") << QByteArray("-0") << qulonglong(0);
    QTest::newRow(" 1") << QByteArray(" 1") << qulonglong(1);
    QTest::newRow(" \\r\\n\\r\\n123") << QByteArray(" \r\n\r\n123") << qulonglong(123);

    // bit boundary tests
    QTest::newRow("127") << QByteArray("127") << qulonglong(127);
    QTest::newRow("128") << QByteArray("128") << qulonglong(128);
    QTest::newRow("129") << QByteArray("129") << qulonglong(129);
    QTest::newRow("-127") << QByteArray("-127") << qulonglong(-127);
    QTest::newRow("-128") << QByteArray("-128") << qulonglong(-128);
    QTest::newRow("-129") << QByteArray("-129") << qulonglong(-129);
    QTest::newRow("32767") << QByteArray("32767") << qulonglong(32767);
    QTest::newRow("32768") << QByteArray("32768") << qulonglong(32768);
    QTest::newRow("32769") << QByteArray("32769") << qulonglong(32769);
    QTest::newRow("-32767") << QByteArray("-32767") << qulonglong(-32767);
    QTest::newRow("-32768") << QByteArray("-32768") << qulonglong(-32768);
    QTest::newRow("-32769") << QByteArray("-32769") << qulonglong(-32769);
    QTest::newRow("65537") << QByteArray("65537") << qulonglong(65537);
    QTest::newRow("65536") << QByteArray("65536") << qulonglong(65536);
    QTest::newRow("65535") << QByteArray("65535") << qulonglong(65535);
    QTest::newRow("-65537") << QByteArray("-65537") << qulonglong(-65537);
    QTest::newRow("-65536") << QByteArray("-65536") << qulonglong(-65536);
    QTest::newRow("-65535") << QByteArray("-65535") << qulonglong(-65535);
    QTest::newRow("2147483646") << QByteArray("2147483646") << qulonglong(2147483646);
    QTest::newRow("2147483647") << QByteArray("2147483647") << qulonglong(2147483647);
    QTest::newRow("2147483648") << QByteArray("2147483648") << Q_UINT64_C(2147483648);
    QTest::newRow("-2147483646") << QByteArray("-2147483646") << qulonglong(-2147483646);
    QTest::newRow("-2147483647") << QByteArray("-2147483647") << qulonglong(-2147483647);
    QTest::newRow("-2147483648") << QByteArray("-2147483648") << quint64(-2147483648LL);
    QTest::newRow("4294967296") << QByteArray("4294967296") << Q_UINT64_C(4294967296);
    QTest::newRow("4294967297") << QByteArray("4294967297") << Q_UINT64_C(4294967297);
    QTest::newRow("4294967298") << QByteArray("4294967298") << Q_UINT64_C(4294967298);
    QTest::newRow("-4294967296") << QByteArray("-4294967296") << quint64(-4294967296);
    QTest::newRow("-4294967297") << QByteArray("-4294967297") << quint64(-4294967297);
    QTest::newRow("-4294967298") << QByteArray("-4294967298") << quint64(-4294967298);
    QTest::newRow("9223372036854775807") << QByteArray("9223372036854775807") << Q_UINT64_C(9223372036854775807);
    QTest::newRow("9223372036854775808") << QByteArray("9223372036854775808") << Q_UINT64_C(9223372036854775808);
    QTest::newRow("9223372036854775809") << QByteArray("9223372036854775809") << Q_UINT64_C(9223372036854775809);
    QTest::newRow("18446744073709551615") << QByteArray("18446744073709551615") << Q_UINT64_C(18446744073709551615);
    QTest::newRow("18446744073709551616") << QByteArray("18446744073709551616") << Q_UINT64_C(0);
    QTest::newRow("18446744073709551617") << QByteArray("18446744073709551617") << Q_UINT64_C(1);
    // 18446744073709551617 bytes should be enough for anyone.... ;-)

    // hex tests
    QTest::newRow("0x0") << QByteArray("0x0") << qulonglong(0);
    QTest::newRow("0x") << QByteArray("0x") << qulonglong(0);
    QTest::newRow("0x1") << QByteArray("0x1") << qulonglong(1);
    QTest::newRow("0xf") << QByteArray("0xf") << qulonglong(15);
    QTest::newRow("0xdeadbeef") << QByteArray("0xdeadbeef") << Q_UINT64_C(3735928559);
    QTest::newRow("0XDEADBEEF") << QByteArray("0XDEADBEEF") << Q_UINT64_C(3735928559);
    QTest::newRow("0xdeadbeefZzzzz") << QByteArray("0xdeadbeefZzzzz") << Q_UINT64_C(3735928559);
    QTest::newRow("  0xdeadbeefZzzzz") << QByteArray("  0xdeadbeefZzzzz") << Q_UINT64_C(3735928559);

    // oct tests
    QTest::newRow("00") << QByteArray("00") << qulonglong(0);
    QTest::newRow("0141") << QByteArray("0141") << qulonglong(97);
    QTest::newRow("01419999") << QByteArray("01419999") << qulonglong(97);
    QTest::newRow("  01419999") << QByteArray("  01419999") << qulonglong(97);

    // bin tests
    QTest::newRow("0b0") << QByteArray("0b0") << qulonglong(0);
    QTest::newRow("0b1") << QByteArray("0b1") << qulonglong(1);
    QTest::newRow("0b10") << QByteArray("0b10") << qulonglong(2);
    QTest::newRow("0B10") << QByteArray("0B10") << qulonglong(2);
    QTest::newRow("0b101010") << QByteArray("0b101010") << qulonglong(42);
    QTest::newRow("0b1010102345") << QByteArray("0b1010102345") << qulonglong(42);
    QTest::newRow("  0b1010102345") << QByteArray("  0b1010102345") << qulonglong(42);

    // utf-16 tests
    if (!for_QString) {
        QTest::newRow("utf16-BE (empty)") << QByteArray("\xfe\xff", 2) << qulonglong(0);
        QTest::newRow("utf16-BE (0xdeadbeef)")
            << QByteArray("\xfe\xff"
                          "\x00\x30\x00\x78\x00\x64\x00\x65\x00\x61\x00\x64\x00\x62\x00\x65\x00\x65\x00\x66", 22)
            << Q_UINT64_C(3735928559);
        QTest::newRow("utf16-LE (empty)") << QByteArray("\xff\xfe", 2) << Q_UINT64_C(0);
        QTest::newRow("utf16-LE (0xdeadbeef)")
            << QByteArray("\xff\xfe"
                          "\x30\x00\x78\x00\x64\x00\x65\x00\x61\x00\x64\x00\x62\x00\x65\x00\x65\x00\x66\x00", 22)
            << Q_UINT64_C(3735928559);
    }
}

// ------------------------------------------------------------------------------
template <typename Whole>
void tst_QTextStream::integral_read_operator_FromDevice() const
{
    QFETCH(QByteArray, input);
    QFETCH(qulonglong, output);
    QTextStream stream(&input);
    Whole sh;
    stream >> sh;
    QCOMPARE(sh, Whole(output));
}

// ------------------------------------------------------------------------------
void tst_QTextStream::generateRealNumbersData(bool for_QString) const
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<double>("output");

    QTest::newRow("empty") << QByteArray() << 0.0;
    QTest::newRow("a") << QByteArray("a") << 0.0;
    QTest::newRow("1.0") << QByteArray("1.0") << 1.0;
    QTest::newRow(" 1") << QByteArray(" 1") << 1.0;
    QTest::newRow(" \\r\\n1.2") << QByteArray(" \r\n1.2") << 1.2;
    QTest::newRow("3.14") << QByteArray("3.14") << 3.14;
    QTest::newRow("-3.14") << QByteArray("-3.14") << -3.14;
    QTest::newRow(" -3.14") << QByteArray(" -3.14") << -3.14;
    QTest::newRow("314e-02") << QByteArray("314e-02") << 3.14;
    QTest::newRow("314E-02") << QByteArray("314E-02") << 3.14;
    QTest::newRow("314e+02") << QByteArray("314e+02") << 31400.;
    QTest::newRow("314E+02") << QByteArray("314E+02") << 31400.;

    // ### add numbers with exponents

    if (!for_QString) {
        QTest::newRow("utf16-BE (empty)") << QByteArray("\xff\xfe", 2) << 0.0;
        QTest::newRow("utf16-LE (empty)") << QByteArray("\xfe\xff", 2) << 0.0;
    }
}

// ------------------------------------------------------------------------------
template <typename Real>
void tst_QTextStream::real_read_operator_FromDevice() const
{
    QFETCH(QByteArray, input);
    QFETCH(double, output);
    QTextStream stream(&input);
    Real sh;
    stream >> sh;
    QCOMPARE(sh, Real(output));
}

// ------------------------------------------------------------------------------
void tst_QTextStream::generateStringData(bool for_QString) const
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QByteArray>("array_output");
    QTest::addColumn<QString>("string_output");
    QTest::addColumn<bool>("status");

    QTest::newRow("empty") << QByteArray() << QByteArray() << QString() << false;
    QTest::newRow("a") << QByteArray("a") << QByteArray("a") << QString("a") << true;
    QTest::newRow("a b") << QByteArray("a b") << QByteArray("a") << QString("a") << true;
    QTest::newRow(" a b") << QByteArray(" a b") << QByteArray("a") << QString("a") << true;
    QTest::newRow("a1") << QByteArray("a1") << QByteArray("a1") << QString("a1") << true;
    QTest::newRow("a1 b1") << QByteArray("a1 b1") << QByteArray("a1") << QString("a1") << true;
    QTest::newRow(" a1 b1") << QByteArray(" a1 b1") << QByteArray("a1") << QString("a1") << true;
    QTest::newRow("\\n\\n\\nole i dole\\n") << QByteArray("\n\n\nole i dole\n")
                                            << QByteArray("ole") << QString("ole") << true;

    if (!for_QString) {
        QTest::newRow("utf16-BE (empty)") << QByteArray("\xff\xfe", 2) << QByteArray()
                                          << QString() << false;
        QTest::newRow("utf16-BE (corrupt)") << QByteArray("\xff", 1) << QByteArray("\xc3\xbf")
                                            << QString::fromUtf8("\xc3\xbf") << true;
        QTest::newRow("utf16-LE (empty)") << QByteArray("\xfe\xff", 2) << QByteArray()
                                          << QString() << false;
        QTest::newRow("utf16-LE (corrupt)") << QByteArray("\xfe", 1) << QByteArray("\xc3\xbe")
                                            << QString::fromUtf8("\xc3\xbe") << true;
    }
}

// ------------------------------------------------------------------------------
void tst_QTextStream::charPtr_read_operator_FromDevice()
{
    QFETCH(QByteArray, input);
    QFETCH(QByteArray, array_output);
    QFETCH(bool, status);

    QBuffer buffer(&input);
    buffer.open(QBuffer::ReadOnly);
    QTextStream stream(&buffer);
    stream.setEncoding(QStringConverter::Latin1);
    stream.setAutoDetectUnicode(true);

    char buf[1024];
    QCOMPARE(static_cast<bool>(stream >> buf), status);

    QCOMPARE((const char *)buf, array_output.constData());
}

// ------------------------------------------------------------------------------
void tst_QTextStream::stringRef_read_operator_FromDevice()
{
    QFETCH(QByteArray, input);
    QFETCH(QString, string_output);
    QFETCH(bool, status);

    QBuffer buffer(&input);
    buffer.open(QBuffer::ReadOnly);
    QTextStream stream(&buffer);
    stream.setEncoding(QStringConverter::Latin1);
    stream.setAutoDetectUnicode(true);

    QString tmp;
    QCOMPARE(static_cast<bool>(stream >> tmp), status);

    QCOMPARE(tmp, string_output);
}

// ------------------------------------------------------------------------------
void tst_QTextStream::byteArray_read_operator_FromDevice()
{
    QFETCH(QByteArray, input);
    QFETCH(QByteArray, array_output);
    QFETCH(bool, status);

    QBuffer buffer(&input);
    buffer.open(QBuffer::ReadOnly);
    QTextStream stream(&buffer);
    stream.setEncoding(QStringConverter::Latin1);
    stream.setAutoDetectUnicode(true);

    QByteArray array;
    QCOMPARE(static_cast<bool>(stream >> array), status);

    QCOMPARE(array, array_output);
}

// ------------------------------------------------------------------------------
template <typename Whole>
void tst_QTextStream::integral_write_operator_ToDevice() const
{
    QFETCH(qulonglong, number);
    QFETCH(QByteArray, data);
    QFETCH(QByteArray, dataWithSeparators);

    QBuffer buffer;
    buffer.open(QBuffer::WriteOnly);
    QTextStream stream(&buffer);
    stream.setLocale(QLocale::c());
    QVERIFY(stream << Whole(number));
    stream.flush();
    QCOMPARE(buffer.data(), data);

    QLocale locale("en-US");
    buffer.reset(); buffer.buffer().clear();
    stream.setLocale(locale);
    QVERIFY(stream << Whole(number));
    stream.flush();
    QCOMPARE(buffer.data(), dataWithSeparators);

    locale.setNumberOptions(QLocale::OmitGroupSeparator);
    buffer.reset(); buffer.buffer().clear();
    stream.setLocale(locale);
    QVERIFY(stream << Whole(number));
    stream.flush();
    QCOMPARE(buffer.data(), data);

    locale = QLocale("de-DE");
    buffer.reset(); buffer.buffer().clear();
    stream.setLocale(locale);
    QVERIFY(stream << Whole(number));
    stream.flush();
    QCOMPARE(buffer.data(), dataWithSeparators.replace(',', '.'));
}

// ------------------------------------------------------------------------------
void tst_QTextStream::signedShort_write_operator_ToDevice_data()
{
    QTest::addColumn<qulonglong>("number");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QByteArray>("dataWithSeparators");

    QTest::newRow("0") << Q_UINT64_C(0) << QByteArray("0") << QByteArray("0");
    QTest::newRow("1") << Q_UINT64_C(1) << QByteArray("1") << QByteArray("1");
    QTest::newRow("-1") << quint64(-1) << QByteArray("-1") << QByteArray("-1");
    QTest::newRow("32767") << Q_UINT64_C(32767) << QByteArray("32767") << QByteArray("32,767");
    QTest::newRow("32768") << Q_UINT64_C(32768) << QByteArray("-32768") << QByteArray("-32,768");
    QTest::newRow("32769") << Q_UINT64_C(32769) << QByteArray("-32767") << QByteArray("-32,767");
    QTest::newRow("65535") << Q_UINT64_C(65535) << QByteArray("-1") << QByteArray("-1");
    QTest::newRow("65536") << Q_UINT64_C(65536) << QByteArray("0") << QByteArray("0");
    QTest::newRow("65537") << Q_UINT64_C(65537) << QByteArray("1") << QByteArray("1");
    QTest::newRow("-32768") << quint64(-32768) << QByteArray("-32768") << QByteArray("-32,768");
}

// ------------------------------------------------------------------------------
void tst_QTextStream::unsignedShort_write_operator_ToDevice_data()
{
    QTest::addColumn<qulonglong>("number");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QByteArray>("dataWithSeparators");

    QTest::newRow("0") << Q_UINT64_C(0) << QByteArray("0") << QByteArray("0");
    QTest::newRow("1") << Q_UINT64_C(1) << QByteArray("1") << QByteArray("1");
    QTest::newRow("-1") << quint64(-1) << QByteArray("65535") << QByteArray("65,535");
    QTest::newRow("32767") << Q_UINT64_C(32767) << QByteArray("32767") << QByteArray("32,767");
    QTest::newRow("32768") << Q_UINT64_C(32768) << QByteArray("32768") << QByteArray("32,768");
    QTest::newRow("32769") << Q_UINT64_C(32769) << QByteArray("32769") << QByteArray("32,769");
    QTest::newRow("65535") << Q_UINT64_C(65535) << QByteArray("65535") << QByteArray("65,535");
    QTest::newRow("65536") << Q_UINT64_C(65536) << QByteArray("0") << QByteArray("0");
    QTest::newRow("65537") << Q_UINT64_C(65537) << QByteArray("1") << QByteArray("1");
}

// ------------------------------------------------------------------------------
void tst_QTextStream::signedInt_write_operator_ToDevice_data()
{
    QTest::addColumn<qulonglong>("number");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QByteArray>("dataWithSeparators");

    QTest::newRow("0") << Q_UINT64_C(0) << QByteArray("0") << QByteArray("0");
    QTest::newRow("1") << Q_UINT64_C(1) << QByteArray("1") << QByteArray("1");
    QTest::newRow("-1") << quint64(-1) << QByteArray("-1") << QByteArray("-1");
    QTest::newRow("32767") << Q_UINT64_C(32767) << QByteArray("32767") << QByteArray("32,767");
    QTest::newRow("32768") << Q_UINT64_C(32768) << QByteArray("32768") << QByteArray("32,768");
    QTest::newRow("32769") << Q_UINT64_C(32769) << QByteArray("32769") << QByteArray("32,769");
    QTest::newRow("65535") << Q_UINT64_C(65535) << QByteArray("65535") << QByteArray("65,535");
    QTest::newRow("65536") << Q_UINT64_C(65536) << QByteArray("65536") << QByteArray("65,536");
    QTest::newRow("65537") << Q_UINT64_C(65537) << QByteArray("65537") << QByteArray("65,537");
    QTest::newRow("2147483647") << Q_UINT64_C(2147483647) << QByteArray("2147483647") << QByteArray("2,147,483,647");
    QTest::newRow("2147483648") << Q_UINT64_C(2147483648) << QByteArray("-2147483648") << QByteArray("-2,147,483,648");
    QTest::newRow("2147483649") << Q_UINT64_C(2147483649) << QByteArray("-2147483647") << QByteArray("-2,147,483,647");
    QTest::newRow("4294967295") << Q_UINT64_C(4294967295) << QByteArray("-1") << QByteArray("-1");
    QTest::newRow("4294967296") << Q_UINT64_C(4294967296) << QByteArray("0") << QByteArray("0");
    QTest::newRow("4294967297") << Q_UINT64_C(4294967297) << QByteArray("1") << QByteArray("1");
    QTest::newRow("-2147483648") << quint64(-2147483648) << QByteArray("-2147483648") << QByteArray("-2,147,483,648");
}

// ------------------------------------------------------------------------------
void tst_QTextStream::unsignedInt_write_operator_ToDevice_data()
{
    QTest::addColumn<qulonglong>("number");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QByteArray>("dataWithSeparators");

    QTest::newRow("0") << Q_UINT64_C(0) << QByteArray("0") << QByteArray("0");
    QTest::newRow("1") << Q_UINT64_C(1) << QByteArray("1") << QByteArray("1");
    QTest::newRow("-1") << quint64(-1) << QByteArray("4294967295") << QByteArray("4,294,967,295");
    QTest::newRow("32767") << Q_UINT64_C(32767) << QByteArray("32767") << QByteArray("32,767");
    QTest::newRow("32768") << Q_UINT64_C(32768) << QByteArray("32768") << QByteArray("32,768");
    QTest::newRow("32769") << Q_UINT64_C(32769) << QByteArray("32769") << QByteArray("32,769");
    QTest::newRow("65535") << Q_UINT64_C(65535) << QByteArray("65535") << QByteArray("65,535");
    QTest::newRow("65536") << Q_UINT64_C(65536) << QByteArray("65536") << QByteArray("65,536");
    QTest::newRow("65537") << Q_UINT64_C(65537) << QByteArray("65537") << QByteArray("65,537");
    QTest::newRow("2147483647") << Q_UINT64_C(2147483647) << QByteArray("2147483647") << QByteArray("2,147,483,647");
    QTest::newRow("2147483648") << Q_UINT64_C(2147483648) << QByteArray("2147483648") << QByteArray("2,147,483,648");
    QTest::newRow("2147483649") << Q_UINT64_C(2147483649) << QByteArray("2147483649") << QByteArray("2,147,483,649");
    QTest::newRow("4294967295") << Q_UINT64_C(4294967295) << QByteArray("4294967295") << QByteArray("4,294,967,295");
    QTest::newRow("4294967296") << Q_UINT64_C(4294967296) << QByteArray("0") << QByteArray("0");
    QTest::newRow("4294967297") << Q_UINT64_C(4294967297) << QByteArray("1") << QByteArray("1");
}

// ------------------------------------------------------------------------------
void tst_QTextStream::qlonglong_write_operator_ToDevice_data()
{
    QTest::addColumn<qulonglong>("number");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QByteArray>("dataWithSeparators");

    QTest::newRow("0") << Q_UINT64_C(0) << QByteArray("0") << QByteArray("0");
    QTest::newRow("1") << Q_UINT64_C(1) << QByteArray("1") << QByteArray("1");
    QTest::newRow("-1") << quint64(-1) << QByteArray("-1") << QByteArray("-1");
    QTest::newRow("32767") << Q_UINT64_C(32767) << QByteArray("32767") << QByteArray("32,767");
    QTest::newRow("32768") << Q_UINT64_C(32768) << QByteArray("32768") << QByteArray("32,768");
    QTest::newRow("32769") << Q_UINT64_C(32769) << QByteArray("32769") << QByteArray("32,769");
    QTest::newRow("65535") << Q_UINT64_C(65535) << QByteArray("65535") << QByteArray("65,535");
    QTest::newRow("65536") << Q_UINT64_C(65536) << QByteArray("65536") << QByteArray("65,536");
    QTest::newRow("65537") << Q_UINT64_C(65537) << QByteArray("65537") << QByteArray("65,537");
    QTest::newRow("2147483647") << Q_UINT64_C(2147483647) << QByteArray("2147483647") << QByteArray("2,147,483,647");
    QTest::newRow("2147483648") << Q_UINT64_C(2147483648) << QByteArray("2147483648") << QByteArray("2,147,483,648");
    QTest::newRow("2147483649") << Q_UINT64_C(2147483649) << QByteArray("2147483649") << QByteArray("2,147,483,649");
    QTest::newRow("4294967295") << Q_UINT64_C(4294967295) << QByteArray("4294967295") << QByteArray("4,294,967,295");
    QTest::newRow("4294967296") << Q_UINT64_C(4294967296) << QByteArray("4294967296") << QByteArray("4,294,967,296");
    QTest::newRow("4294967297") << Q_UINT64_C(4294967297) << QByteArray("4294967297") << QByteArray("4,294,967,297");
    QTest::newRow("9223372036854775807") << Q_UINT64_C(9223372036854775807) << QByteArray("9223372036854775807") << QByteArray("9,223,372,036,854,775,807");
    QTest::newRow("9223372036854775808") << Q_UINT64_C(9223372036854775808) << QByteArray("-9223372036854775808") << QByteArray("-9,223,372,036,854,775,808");
    QTest::newRow("9223372036854775809") << Q_UINT64_C(9223372036854775809) << QByteArray("-9223372036854775807") << QByteArray("-9,223,372,036,854,775,807");
    QTest::newRow("18446744073709551615") << Q_UINT64_C(18446744073709551615) << QByteArray("-1") << QByteArray("-1");
    QTest::newRow("-9223372036854775808") << quint64(Q_INT64_C(-9223372036854775807) - 1) << QByteArray("-9223372036854775808") << QByteArray("-9,223,372,036,854,775,808");
}

// ------------------------------------------------------------------------------
void tst_QTextStream::qulonglong_write_operator_ToDevice_data()
{
    QTest::addColumn<qulonglong>("number");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QByteArray>("dataWithSeparators");

    QTest::newRow("0") << Q_UINT64_C(0) << QByteArray("0") << QByteArray("0");
    QTest::newRow("1") << Q_UINT64_C(1) << QByteArray("1") << QByteArray("1");
    QTest::newRow("-1") << quint64(-1) << QByteArray("18446744073709551615") << QByteArray("18,446,744,073,709,551,615");
    QTest::newRow("32767") << Q_UINT64_C(32767) << QByteArray("32767") << QByteArray("32,767");
    QTest::newRow("32768") << Q_UINT64_C(32768) << QByteArray("32768") << QByteArray("32,768");
    QTest::newRow("32769") << Q_UINT64_C(32769) << QByteArray("32769") << QByteArray("32,769");
    QTest::newRow("65535") << Q_UINT64_C(65535) << QByteArray("65535") << QByteArray("65,535");
    QTest::newRow("65536") << Q_UINT64_C(65536) << QByteArray("65536") << QByteArray("65,536");
    QTest::newRow("65537") << Q_UINT64_C(65537) << QByteArray("65537") << QByteArray("65,537");
    QTest::newRow("2147483647") << Q_UINT64_C(2147483647) << QByteArray("2147483647") << QByteArray("2,147,483,647");
    QTest::newRow("2147483648") << Q_UINT64_C(2147483648) << QByteArray("2147483648") << QByteArray("2,147,483,648");
    QTest::newRow("2147483649") << Q_UINT64_C(2147483649) << QByteArray("2147483649") << QByteArray("2,147,483,649");
    QTest::newRow("4294967295") << Q_UINT64_C(4294967295) << QByteArray("4294967295") << QByteArray("4,294,967,295");
    QTest::newRow("4294967296") << Q_UINT64_C(4294967296) << QByteArray("4294967296") << QByteArray("4,294,967,296");
    QTest::newRow("4294967297") << Q_UINT64_C(4294967297) << QByteArray("4294967297") << QByteArray("4,294,967,297");
    QTest::newRow("9223372036854775807") << Q_UINT64_C(9223372036854775807) << QByteArray("9223372036854775807") << QByteArray("9,223,372,036,854,775,807");
    QTest::newRow("9223372036854775808") << Q_UINT64_C(9223372036854775808) << QByteArray("9223372036854775808") << QByteArray("9,223,372,036,854,775,808");
    QTest::newRow("9223372036854775809") << Q_UINT64_C(9223372036854775809) << QByteArray("9223372036854775809") << QByteArray("9,223,372,036,854,775,809");
    QTest::newRow("18446744073709551615") << Q_UINT64_C(18446744073709551615) << QByteArray("18446744073709551615") << QByteArray("18,446,744,073,709,551,615");
}

// ------------------------------------------------------------------------------
void tst_QTextStream::generateRealNumbersDataWrite() const
{
    QTest::addColumn<double>("number");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QByteArray>("dataWithSeparators");

    QTest::newRow("0") << 0.0 << QByteArray("0") << QByteArray("0");
    QTest::newRow("3.14") << 3.14 << QByteArray("3.14") << QByteArray("3.14");
    QTest::newRow("-3.14") << -3.14 << QByteArray("-3.14") << QByteArray("-3.14");
    QTest::newRow("1.2e+10") << 1.2e+10 << QByteArray("1.2e+10") << QByteArray("1.2e+10");
    QTest::newRow("-1.2e+10") << -1.2e+10 << QByteArray("-1.2e+10") << QByteArray("-1.2e+10");
    QTest::newRow("12345") << 12345. << QByteArray("12345") << QByteArray("12,345");
}

// ------------------------------------------------------------------------------
template <typename Real>
void tst_QTextStream::real_write_operator_ToDevice() const
{
    QFETCH(double, number);
    QFETCH(QByteArray, data);
    QFETCH(QByteArray, dataWithSeparators);

    QBuffer buffer;
    buffer.open(QBuffer::WriteOnly);
    QTextStream stream(&buffer);
    stream.setLocale(QLocale::c());
    Real f = Real(number);
    QVERIFY(stream << f);
    stream.flush();
    QCOMPARE(buffer.data(), data);

    buffer.reset();
    stream.setLocale(QLocale("en-US"));
    QVERIFY(stream << f);
    stream.flush();
    QCOMPARE(buffer.data(), dataWithSeparators);
}

// ------------------------------------------------------------------------------
void tst_QTextStream::string_write_operator_ToDevice_data()
{
    QTest::addColumn<QByteArray>("bytedata");
    QTest::addColumn<QString>("stringdata");
    QTest::addColumn<QByteArray>("result");

    QTest::newRow("empty") << QByteArray("", 1) << QString(1, '\0') << QByteArray("", 1);
    QTest::newRow("a") << QByteArray("a") << QString("a") << QByteArray("a");
    QTest::newRow("a cow jumped over the moon")
        << QByteArray("a cow jumped over the moon")
        << QString("a cow jumped over the moon")
        << QByteArray("a cow jumped over the moon");

    // ### get the utf16-be test on its legs.
    /*
    QTest::newRow("utf16-BE (a cow jumped over the moon)")
        << QByteArray("\xff\xfe\x00\x61\x00\x20\x00\x63\x00\x6f\x00\x77\x00\x20\x00\x6a\x00\x75\x00\x6d\x00\x70\x00\x65\x00\x64\x00\x20\x00\x6f\x00\x76\x00\x65\x00\x72\x00\x20\x00\x74\x00\x68\x00\x65\x00\x20\x00\x6d\x00\x6f\x00\x6f\x00\x6e\x00\x0a", 56)
        << QString("a cow jumped over the moon")
        << QByteArray("a cow jumped over the moon");
    */
}

// ------------------------------------------------------------------------------
void tst_QTextStream::string_write_operator_ToDevice()
{
    QFETCH(QByteArray, bytedata);
    QFETCH(QString, stringdata);
    QFETCH(QByteArray, result);

    {
        // char*
        QBuffer buf;
        buf.open(QBuffer::WriteOnly);
        QTextStream stream(&buf);
        stream.setEncoding(QStringConverter::Latin1);
        stream.setAutoDetectUnicode(true);

        QVERIFY(stream << bytedata.constData());
        stream.flush();
        // Size information is discarded, so "empty" doesn't get its (size 1)
        // null byte transmitted, so only compare to constData(), ignoring size.
        QCOMPARE(buf.data().constData(), result.constData());
    }
    {
        // QByteArray
        QBuffer buf;
        buf.open(QBuffer::WriteOnly);
        QTextStream stream(&buf);
        stream.setEncoding(QStringConverter::Latin1);
        stream.setAutoDetectUnicode(true);

        QVERIFY(stream << bytedata);
        stream.flush();
        QCOMPARE(buf.data(), result);
    }
    {
        // QString
        QBuffer buf;
        buf.open(QBuffer::WriteOnly);
        QTextStream stream(&buf);
        stream.setEncoding(QStringConverter::Latin1);
        stream.setAutoDetectUnicode(true);

        QVERIFY(stream << stringdata);
        stream.flush();
        QCOMPARE(buf.data(), result);
    }
}

void tst_QTextStream::latin1String_write_operator_ToDevice()
{
    QBuffer buf;
    buf.open(QBuffer::WriteOnly);
    QTextStream stream(&buf);
    stream.setEncoding(QStringConverter::Latin1);
    stream.setAutoDetectUnicode(true);

    QVERIFY(stream << QLatin1String("No explicit length"));
    QVERIFY(stream << QLatin1String("Explicit length - ignore this part", 15));
    stream.flush();
    QCOMPARE(buf.data(), "No explicit lengthExplicit length");
}

void tst_QTextStream::stringref_write_operator_ToDevice()
{
    QBuffer buf;
    buf.open(QBuffer::WriteOnly);
    QTextStream stream(&buf);
    stream.setEncoding(QStringConverter::Latin1);
    stream.setAutoDetectUnicode(true);

    const QStringView expected = u"No explicit lengthExplicit length";

    QVERIFY(stream << expected.left(18));
    QVERIFY(stream << expected.mid(18));
    stream.flush();
    QCOMPARE(buf.data(), "No explicit lengthExplicit length");
}

void tst_QTextStream::stringview_write_operator_ToDevice()
{
    QBuffer buf;
    buf.open(QBuffer::WriteOnly);
    QTextStream stream(&buf);
    const QStringView expected = u"expectedStringView";
    QVERIFY(stream << expected);
    stream.flush();
    QCOMPARE(buf.data(), "expectedStringView");
}

// ------------------------------------------------------------------------------
void tst_QTextStream::stream_bool_operator_Test()
{
    const QStringView expected = u"expectedStringView";
    QString line("exp");
    QTextStream stream(&line);
    QChar ch1, ch2, ch3;

    QVERIFY(stream >> ch1);
    QCOMPARE(ch1, 'e');
    QVERIFY(stream >> ch2);
    QCOMPARE(ch2, 'x');
    QVERIFY(stream >> ch3);
    QCOMPARE(ch3, 'p');

    QVERIFY(stream << "ectedStringView");
    stream.flush();
    QCOMPARE(*stream.string(), expected);

    QTextStream emptyStream("");
    QVERIFY(!(emptyStream >> ch1));
    QVERIFY(!(emptyStream << "Hello"));

    QTextStream textStream("1 2 3 error");
    int n;
    for (int i = 0; i < 3; ++i) {
        QVERIFY(textStream >> n);
        QCOMPARE_EQ(n, i + 1);
    }
    QVERIFY(!textStream.atEnd());
    QVERIFY(!(textStream >> n));
}

// ------------------------------------------------------------------------------
void tst_QTextStream::useCase1()
{
    QFile::remove("testfile");
    QFile file("testfile");
    QVERIFY(file.open(QFile::ReadWrite));

    {
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Latin1);
        stream.setAutoDetectUnicode(true);

        stream << 4.15 << ' ' << QByteArray("abc") << ' ' << QString("ole");
    }

    file.seek(0);
    QCOMPARE(file.readAll(), "4.15 abc ole");
    file.seek(0);

    {
        double d;
        QByteArray a;
        QString s;
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Latin1);
        stream.setAutoDetectUnicode(true);

        stream >> d;
        stream >> a;
        stream >> s;

        QCOMPARE(d, 4.15);
        QCOMPARE(a, "abc");
        QCOMPARE(s, QString("ole"));
    }
}

// ------------------------------------------------------------------------------
void tst_QTextStream::useCase2()
{
    QFile::remove("testfile");
    QFile file("testfile");
    QVERIFY(file.open(QFile::ReadWrite));

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Latin1);
    stream.setAutoDetectUnicode(true);

    stream << 4.15 << ' ' << QByteArray("abc") << ' ' << QString("ole");

    file.close();
    QVERIFY(file.open(QFile::ReadWrite));

    QCOMPARE(file.readAll(), "4.15 abc ole");

    file.close();
    QVERIFY(file.open(QFile::ReadWrite));

    double d;
    QByteArray a;
    QString s;
    QTextStream stream2(&file);
    stream2.setEncoding(QStringConverter::Latin1);
    stream2.setAutoDetectUnicode(true);

    QVERIFY(stream2 >> d);
    QVERIFY(stream2 >> a);
    QVERIFY(stream2 >> s);

    QCOMPARE(d, 4.15);
    QCOMPARE(a, "abc");
    QCOMPARE(s, "ole");
}

// ------------------------------------------------------------------------------
void tst_QTextStream::manipulators_data()
{
    QTest::addColumn<int>("base");
    QTest::addColumn<QTextStream::FieldAlignment>("alignFlag");
    QTest::addColumn<QTextStream::NumberFlags>("numberFlag");
    QTest::addColumn<int>("width");
    QTest::addColumn<double>("realNumber");
    QTest::addColumn<qlonglong>("intNumber");
    QTest::addColumn<QString>("textData");
    QTest::addColumn<QByteArray>("result");

    QTest::newRow("no flags")
        << 10 << QTextStream::AlignLeft << QTextStream::NumberFlags{}
        << 0  << 5.0 << 5LL << QString("five") << QByteArray("55five");
    QTest::newRow("rightadjust")
        << 10 << QTextStream::AlignRight << QTextStream::NumberFlags{}
        << 10 << 5.0 << 5LL << QString("five") << QByteArray("         5         5      five");
    QTest::newRow("leftadjust")
        << 10 << QTextStream::AlignLeft << QTextStream::NumberFlags{}
        << 10 << 5.0 << 5LL << QString("five") << QByteArray("5         5         five      ");
    QTest::newRow("showpos-wide")
        << 10 << QTextStream::AlignRight << QTextStream::NumberFlags{QTextStream::ForceSign}
        << 10 << 5.0 << 5LL << QString("five") << QByteArray("        +5        +5      five");
    QTest::newRow("showpos-pi")
        << 10 << QTextStream::AlignRight << QTextStream::NumberFlags{QTextStream::ForceSign}
        << 5 << 3.14 << -5LL << QString("five") << QByteArray("+3.14   -5 five");
    QTest::newRow("min-value")
        << 10 << QTextStream::AlignRight << QTextStream::NumberFlags{} << 5
        << 3.14 << (std::numeric_limits<qlonglong>::min)()
        << QString("five") << QByteArray(" 3.14-9223372036854775808 five");
    QTest::newRow("hex-lower")
        << 16 << QTextStream::AlignRight << QTextStream::NumberFlags{QTextStream::ShowBase}
        << 5 << 3.14 << -5LL << QString("five") << QByteArray(" 3.14 -0x5 five");
    QTest::newRow("hex-upper")
        << 16 << QTextStream::AlignRight
        << (QTextStream::ShowBase | QTextStream::UppercaseBase)
        << 5 << 3.14 << -5LL << QString("five") << QByteArray(" 3.14 -0X5 five");
    QTest::newRow("hex-negative")
        << 16 << QTextStream::AlignRight << (QTextStream::ShowBase | QTextStream::ForceSign)
        << 5 << 3.14 << -5LL << QString("five") << QByteArray("+3.14 -0x5 five");
}

// ------------------------------------------------------------------------------
void tst_QTextStream::manipulators()
{
    QFETCH(int, base);
    QFETCH(QTextStream::FieldAlignment, alignFlag);
    QFETCH(QTextStream::NumberFlags, numberFlag);
    QFETCH(int, width);
    QFETCH(double, realNumber);
    QFETCH(qlonglong, intNumber);
    QFETCH(QString, textData);
    QFETCH(QByteArray, result);

    QBuffer buffer;
    buffer.open(QBuffer::WriteOnly);

    QTextStream stream(&buffer);
    stream.setEncoding(QStringConverter::Latin1);
    stream.setAutoDetectUnicode(true);

    stream.setIntegerBase(base);
    stream.setFieldAlignment(alignFlag);
    stream.setNumberFlags(numberFlag);
    stream.setFieldWidth(width);
    QVERIFY(stream << realNumber);
    QVERIFY(stream << intNumber);
    QVERIFY(stream << textData);
    stream.flush();

    QCOMPARE(buffer.data(), result);
}

void tst_QTextStream::generateBOM()
{
    QFile::remove("bom.txt");
    {
        QFile file("bom.txt");
        QVERIFY(file.open(QFile::ReadWrite | QFile::Truncate));

        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf16LE);
        stream << "Hello" << Qt::endl;

        file.close();
        QVERIFY(file.open(QFile::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("\x48\x00\x65\00\x6c\00\x6c\00\x6f\x00\x0a\x00", 12));
    }

    QFile::remove("bom.txt");
    {
        QFile file("bom.txt");
        QVERIFY(file.open(QFile::ReadWrite | QFile::Truncate));

        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf16LE);
        QVERIFY(stream << Qt::bom << "Hello" << Qt::endl);

        file.close();
        QVERIFY(file.open(QFile::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("\xff\xfe\x48\x00\x65\00\x6c\00\x6c\00\x6f\x00\x0a\x00", 14));
    }
}

void tst_QTextStream::readBomSeekBackReadBomAgain()
{
    QFile::remove("utf8bom");
    QFile file("utf8bom");
    QVERIFY(file.open(QFile::ReadWrite));
    file.write("\xef\xbb\xbf""Andreas");
    file.seek(0);
    QCOMPARE(file.pos(), qint64(0));

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    QString Andreas;
    QVERIFY(stream >> Andreas);
    QCOMPARE(Andreas, QString("Andreas"));
    stream.seek(0);
    QVERIFY(stream >> Andreas);
    QCOMPARE(Andreas, QString("Andreas"));
}

// ------------------------------------------------------------------------------
void tst_QTextStream::status_real_read_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<double>("expected_f");
    QTest::addColumn<QString>("expected_w");
    QTest::addColumn<QList<int> >("results");

    QTest::newRow("1.23 abc   ") << QString("1.23 abc   ") << 1.23 << QString("abc")
                              << (QList<int>()
                                  << (int)QTextStream::Ok
                                  << (int)QTextStream::ReadCorruptData
                                  << (int)QTextStream::Ok
                                  << (int)QTextStream::Ok
                                  << (int)QTextStream::ReadPastEnd);
}

void tst_QTextStream::status_real_read()
{
    QFETCH(QString, input);
    QFETCH(double, expected_f);
    QFETCH(QString, expected_w);
    QFETCH(QList<int>, results);

    QTextStream s(&input);
    double f = 0.0;
    QString w;
    s >> f;
    QCOMPARE((int)s.status(), results.at(0));
    QCOMPARE(f, expected_f);
    s >> f;
    QCOMPARE((int)s.status(), results.at(1));
    s.resetStatus();
    QCOMPARE((int)s.status(), results.at(2));
    s >> w;
    QCOMPARE((int)s.status(), results.at(3));
    QCOMPARE(w, expected_w);
    s >> f;
    QCOMPARE((int)s.status(), results.at(4));
}

void tst_QTextStream::status_integer_read()
{
    QTextStream s("123 abc   ");
    int i;
    QString w;
    QVERIFY(s >> i);
    QCOMPARE(s.status(), QTextStream::Ok);
    QVERIFY(!(s >> i));
    QCOMPARE(s.status(), QTextStream::ReadCorruptData);
    s.resetStatus();
    QCOMPARE(s.status(), QTextStream::Ok);
    QVERIFY(s >> w);
    QCOMPARE(s.status(), QTextStream::Ok);
    QCOMPARE(w, QString("abc"));
    QVERIFY(!(s >> i));
    QCOMPARE(s.status(), QTextStream::ReadPastEnd);
}

void tst_QTextStream::status_word_read()
{
    QTextStream s("abc ");
    QString w;
    QVERIFY(s >> w);
    QCOMPARE(s.status(), QTextStream::Ok);
    QVERIFY(!(s >> w));
    QCOMPARE(s.status(), QTextStream::ReadPastEnd);
}

class FakeBuffer : public QBuffer
{
protected:
    qint64 writeData(const char *c, qint64 i) override { return m_lock ? 0 : QBuffer::writeData(c, i); }
public:
    FakeBuffer(bool locked = false) : m_lock(locked) {}
    void setLocked(bool locked) { m_lock = locked; }
private:
    bool m_lock;
};

void tst_QTextStream::status_write_error()
{
    FakeBuffer fb(false);
    QVERIFY(fb.open(QBuffer::ReadWrite));
    QTextStream fs(&fb);
    fs.setEncoding(QStringConverter::Latin1);
    /* first write some initial content */
    QVERIFY(fs << "hello");
    fs.flush();
    QCOMPARE(fs.status(), QTextStream::Ok);
    QCOMPARE(fb.data(), "hello");
    /* then test that writing can cause an error */
    fb.setLocked(true);
    QVERIFY(fs << "error");
    fs.flush();
    QCOMPARE(fs.status(), QTextStream::WriteFailed);
    QCOMPARE(fb.data(), "hello");
    /* finally test that writing after an error doesn't change the stream any more */
    fb.setLocked(false);
    fs << "can't do that";
    fs.flush();
    QCOMPARE(fs.status(), QTextStream::WriteFailed);
    QCOMPARE(fb.data(), "hello");
}

void tst_QTextStream::alignAccountingStyle()
{
    {
    QString result;
    QTextStream out(&result);
    out.setFieldAlignment(QTextStream::AlignAccountingStyle);
    out.setFieldWidth(4);
    out.setPadChar('0');
    out << -1;
    QCOMPARE(result, QLatin1String("-001"));
    }

    {
    QString result;
    QTextStream out(&result);
    out.setFieldAlignment(QTextStream::AlignAccountingStyle);
    out.setFieldWidth(4);
    out.setPadChar('0');
    QVERIFY(out << "-1");
    QCOMPARE(result, QLatin1String("00-1"));
    }

    {
    QString result;
    QTextStream out(&result);
    out.setFieldAlignment(QTextStream::AlignAccountingStyle);
    out.setFieldWidth(6);
    out.setPadChar('0');
    QVERIFY(out << -1.2);
    QCOMPARE(result, QLatin1String("-001.2"));
    }

    {
    QString result;
    QTextStream out(&result);
    out.setFieldAlignment(QTextStream::AlignAccountingStyle);
    out.setFieldWidth(6);
    out.setPadChar('0');
    QVERIFY(out << "-1.2");
    QCOMPARE(result, QLatin1String("00-1.2"));
    }
}

void tst_QTextStream::setEncoding()
{
    QByteArray ba("\xe5 v\xe6r\n\xc3\xa5 v\xc3\xa6r\n");
    QString res = QLatin1String("\xe5 v\xe6r");

    QTextStream stream(ba);
    stream.setEncoding(QStringConverter::Latin1);
    QCOMPARE(stream.readLine(),res);
    stream.setEncoding(QStringConverter::Utf8);
    QCOMPARE(stream.readLine(),res);
}

void tst_QTextStream::double_write_with_flags_data()
{
    QTest::addColumn<double>("number");
    QTest::addColumn<QString>("output");
    QTest::addColumn<int>("numberFlags");
    QTest::addColumn<int>("realNumberNotation");

    QTest::newRow("-ForceSign") << -1.23 << QString("-1.23") << (int)QTextStream::ForceSign << 0;
    QTest::newRow("+ForceSign") << 1.23 << QString("+1.23") << (int)QTextStream::ForceSign << 0;
    QTest::newRow("inf") << qInf() << QString("inf") << 0 << 0;
    QTest::newRow("-inf") << -qInf() << QString("-inf") << 0 << 0;
    QTest::newRow("inf uppercase") << qInf() << QString("INF") << (int)QTextStream::UppercaseDigits << 0;
    QTest::newRow("-inf uppercase") << -qInf() << QString("-INF") << (int)QTextStream::UppercaseDigits << 0;
    QTest::newRow("nan") << qQNaN() << QString("nan") << 0 << 0;
    QTest::newRow("NAN") << qQNaN() << QString("NAN") << (int)QTextStream::UppercaseDigits << 0;
    QTest::newRow("scientific") << 1.234567e+02 << QString("1.234567e+02") << 0  << (int)QTextStream::ScientificNotation;
    QTest::newRow("scientific2") << 1.234567e+02 << QString("1.234567e+02") << (int)QTextStream::UppercaseBase << (int)QTextStream::ScientificNotation;
    QTest::newRow("scientific uppercase") << 1.234567e+02 << QString("1.234567E+02") << (int)QTextStream::UppercaseDigits << (int)QTextStream::ScientificNotation;
}

void tst_QTextStream::double_write_with_flags()
{
    QFETCH(double, number);
    QFETCH(QString, output);
    QFETCH(int, numberFlags);
    QFETCH(int, realNumberNotation);

    QString buf;
    QTextStream stream(&buf);
    if (numberFlags)
        stream.setNumberFlags(QTextStream::NumberFlag(numberFlags));
    if (realNumberNotation)
        stream.setRealNumberNotation(QTextStream::RealNumberNotation(realNumberNotation));
    QVERIFY(stream << number);
    QCOMPARE(buf, output);
}

void tst_QTextStream::double_write_with_precision_data()
{
    QTest::addColumn<int>("precision");
    QTest::addColumn<double>("value");
    QTest::addColumn<QString>("result");

    QTest::ignoreMessage(QtWarningMsg, "QTextStream::setRealNumberPrecision: Invalid precision (-1)");
    QTest::newRow("-1") << -1 << 3.14159 << QString("3.14159");
    QTest::newRow("0") << 0 << 3.14159 << QString("3");
    QTest::newRow("1") << 1 << 3.14159 << QString("3");
    QTest::newRow("2") << 2 << 3.14159 << QString("3.1");
    QTest::newRow("3") << 3 << 3.14159 << QString("3.14");
    QTest::newRow("5") << 5 << 3.14159 << QString("3.1416");
    QTest::newRow("6") << 6 << 3.14159 << QString("3.14159");
    QTest::newRow("7") << 7 << 3.14159 << QString("3.14159");
    QTest::newRow("10") << 10 << 3.14159 << QString("3.14159");
}

void tst_QTextStream::double_write_with_precision()
{
    QFETCH(int, precision);
    QFETCH(double, value);
    QFETCH(QString, result);

    QString buf;
    QTextStream stream(&buf);
    stream.setRealNumberPrecision(precision);
    QVERIFY(stream << value);
    QCOMPARE(buf, result);
}

void tst_QTextStream::int_read_with_locale_data()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("output");
    QTest::addColumn<bool>("status");

    QTest::newRow("C -123") << QString("C") << QString("-123") << -123 << true;
    QTest::newRow("C +123") << QString("C") << QString("+123") << 123 << true;
    QTest::newRow("C 12345") << QString("C") << QString("12345") << 12345 << true;
    QTest::newRow("C 12,345") << QString("C") << QString("12,345") << 12 << true;
    QTest::newRow("C 12.345") << QString("C") << QString("12.345") << 12 << true;

    QTest::newRow("de_DE -123") << QString("de_DE") << QString("-123") << -123 << true;
    QTest::newRow("de_DE +123") << QString("de_DE") << QString("+123") << 123 << true;
    QTest::newRow("de_DE 12345") << QString("de_DE") << QString("12345") << 12345 << true;
    QTest::newRow("de_DE 12.345") << QString("de_DE") << QString("12.345") << 12345 << true;
    QTest::newRow("de_DE .12345") << QString("de_DE") << QString(".12345") << 0 << false;
}

void tst_QTextStream::int_read_with_locale()
{
    QFETCH(QString, locale);
    QFETCH(QString, input);
    QFETCH(int, output);
    QFETCH(bool, status);

    QTextStream stream(&input);
    stream.setLocale(QLocale(locale));
    int result;
    QCOMPARE(static_cast<bool>(stream >> result), status);
    QCOMPARE(result, output);
}

void tst_QTextStream::int_write_with_locale_data()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<int>("numberFlags");
    QTest::addColumn<int>("input");
    QTest::addColumn<QString>("output");
    QTest::addColumn<int>("fieldWidth");
    QTest::addColumn<QTextStream::FieldAlignment>("fieldAlignment");

    const auto alignDefault = QTextStream().fieldAlignment();
    constexpr int forceSign = QTextStream::ForceSign;

    QTest::newRow("C -123") << u"C"_s << 0 << -123 << u"-123"_s << 0 << alignDefault;
    QTest::newRow("C +123") << u"C"_s << forceSign << 123 << u"+123"_s << 0 << alignDefault;
    QTest::newRow("C 12345") << u"C"_s << 0 << 12345 << u"12345"_s << 0 << alignDefault;

    QTest::newRow("de_DE -123") << u"de_DE"_s << 0 << -123 << u"-123"_s << 0 << alignDefault;
    QTest::newRow("de_DE +123") << u"de_DE"_s << forceSign << 123 << u"+123"_s << 0 << alignDefault;
    QTest::newRow("de_DE 12345") << u"de_DE"_s << 0 << 12345 << u"12.345"_s << 0 << alignDefault;

    constexpr auto alignAccountingStyle = QTextStream::FieldAlignment::AlignAccountingStyle;

    {
        const QLocale loc("ar_EG"_L1);
        // Arabic as spoken in Egypt has a two-code-point negativeSign():
        const auto minus = loc.negativeSign();
        QCOMPARE(minus.size(), 2);
        // ditto positiveSign():
        const auto plus = loc.positiveSign();
        QCOMPARE(plus.size(), 2);

        QTest::addRow("ar_EG -123") << u"ar_EG"_s << 0 << -123
                                    << (minus + u"     ١٢٣")
                                    << 10 << alignAccountingStyle;
        QTest::newRow("ar_EG +123") << u"ar_EG"_s << forceSign << 123
                                    << (plus + u"     ١٢٣")
                                    << 10 << alignAccountingStyle;
        QTest::newRow("ar_EG 12345") << u"ar_EG"_s << 0 << 12345
                                     << u"    ١٢٬٣٤٥"_s
                                     << 10 << alignAccountingStyle;
    }
}

void tst_QTextStream::int_write_with_locale()
{
    QFETCH(QString, locale);
    QFETCH(int, numberFlags);
    QFETCH(int, input);
    QFETCH(QString, output);
    QFETCH(const int, fieldWidth);
    QFETCH(const QTextStream::FieldAlignment, fieldAlignment);

    QString result;
    QTextStream stream(&result);
    stream.setLocale(QLocale(locale));
    stream.setFieldAlignment(fieldAlignment);
    if (numberFlags)
        stream.setNumberFlags(QTextStream::NumberFlags(numberFlags));
    if (fieldWidth)
        stream.setFieldWidth(fieldWidth);

    QVERIFY(stream << input);
    QCOMPARE(result, output);
}

void tst_QTextStream::textModeOnEmptyRead()
{
    const QString filename(tempDir.path() + QLatin1String("/textmodetest.txt"));

    QFile file(filename);
    QVERIFY2(file.open(QIODevice::ReadWrite | QIODevice::Text), qPrintable(file.errorString()));
    QTextStream stream(&file);
    QVERIFY(file.isTextModeEnabled());
    QString emptyLine = stream.readLine(); // Text mode flag cleared here
    QVERIFY(file.isTextModeEnabled());
}

void tst_QTextStream::autodetectUnicode_data()
{
    QTest::addColumn<QStringConverter::Encoding>("encoding");
    QTest::newRow("Utf8") << QStringConverter::Utf8;
    QTest::newRow("Utf16BE") << QStringConverter::Utf16BE;
    QTest::newRow("Utf16LE") << QStringConverter::Utf16LE;
    QTest::newRow("Utf32BE") << QStringConverter::Utf32BE;
    QTest::newRow("Utf32LE") << QStringConverter::Utf32LE;
}

void tst_QTextStream::autodetectUnicode()
{
    QFETCH(QStringConverter::Encoding, encoding);

    QTemporaryFile file;
    QVERIFY(file.open());

    QString original("HelloWorld👋");

    {
        QTextStream out(&file);
        out.setGenerateByteOrderMark(true);
        out.setEncoding(encoding);
        out << original;
    }
    file.seek(0);
    {
        QTextStream in(&file);
        QString actual;
        QVERIFY(in >> actual);
        QCOMPARE(actual, original);
        QCOMPARE(in.encoding(), encoding);
    }
    file.seek(0);
    // Again, but change order of calls to QTextStream...
    {
        QTextStream out(&file);
        out.setEncoding(encoding);
        out.setGenerateByteOrderMark(true);
        out << original;
    }
    file.seek(0);
    {
        QTextStream in(&file);
        QString actual;
        QVERIFY(in >> actual);
        QCOMPARE(actual, original);
        QCOMPARE(in.encoding(), encoding);
    }
}


// ------------------------------------------------------------------------------

QTEST_MAIN(tst_QTextStream)
#include "tst_qtextstream.moc"

