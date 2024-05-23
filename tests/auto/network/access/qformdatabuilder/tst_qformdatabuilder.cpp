// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtNetwork/qformdatabuilder.h>

#include <QtCore/qbuffer.h>
#include <QtCore/qfile.h>

#include <QtTest/qtest.h>

using namespace Qt::StringLiterals;

class tst_QFormDataBuilder : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void generateQHttpPartWithDevice_data();
    void generateQHttpPartWithDevice();

    void escapesBackslashAndQuotesInFilenameAndName_data();
    void escapesBackslashAndQuotesInFilenameAndName();

    void picksUtf8EncodingOnlyIfL1OrAsciiDontSuffice_data();
    void picksUtf8EncodingOnlyIfL1OrAsciiDontSuffice();
};

void tst_QFormDataBuilder::generateQHttpPartWithDevice_data()
{
    QTest::addColumn<QLatin1StringView>("name_data");
    QTest::addColumn<QString>("real_file_name");
    QTest::addColumn<QString>("body_name_data");
    QTest::addColumn<QByteArray>("expected_content_type_data");
    QTest::addColumn<QByteArray>("expected_content_disposition_data");

    QTest::newRow("txt-ascii") << "text"_L1 << "rfc3252.txt" << "rfc3252.txt" << "text/plain"_ba
                               << "form-data; name=\"text\"; filename=rfc3252.txt"_ba;
    QTest::newRow("txt-latin") << "text"_L1 << "rfc3252.txt" << "szöveg.txt" << "text/plain"_ba
                               << "form-data; name=\"text\"; filename*=ISO-8859-1''sz%F6veg.txt"_ba;
    QTest::newRow("txt-unicode") << "text"_L1 << "rfc3252.txt" << "テキスト.txt" << "text/plain"_ba
                                 << "form-data; name=\"text\"; filename*=UTF-8''%E3%83%86%E3%82%AD%E3%82%B9%E3%83%88.txt"_ba;

    QTest::newRow("jpg-ascii") << "image"_L1 << "image1.jpg" << "image1.jpg" << "image/jpeg"_ba
                               << "form-data; name=\"image\"; filename=image1.jpg"_ba;
    QTest::newRow("jpg-latin") << "image"_L1 << "image1.jpg" << "kép.jpg" << "image/jpeg"_ba
                               << "form-data; name=\"image\"; filename*=ISO-8859-1''k%E9p.jpg"_ba;
    QTest::newRow("jpg-unicode") << "image"_L1 << "image1.jpg" << "絵.jpg" << "image/jpeg"_ba
                                 << "form-data; name=\"image\"; filename*=UTF-8''%E7%B5%B5"_ba;

    QTest::newRow("doc-ascii") << "text"_L1 << "document.docx" << "word.docx"
                               << "application/vnd.openxmlformats-officedocument.wordprocessingml.document"_ba
                               << "form-data; name=\"text\"; filename=word.docx"_ba;
    QTest::newRow("doc-latin") << "text"_L1 << "document.docx" << "szöveg.docx"
                               << "application/vnd.openxmlformats-officedocument.wordprocessingml.document"_ba
                               << "form-data; name=\"text\"; filename*=ISO-8859-1''sz%F6veg.docx"_ba;
    QTest::newRow("doc-unicode") << "text"_L1 << "document.docx" << "テキスト.docx"
                                 << "application/vnd.openxmlformats-officedocument.wordprocessingml.document"_ba
                                 << "form-data; name=\"text\"; filename*=UTF-8''%E3%83%86%E3%82%AD%E3%82%B9%E3%83%88.docx"_ba;

    QTest::newRow("xls-ascii") << "spreadsheet"_L1 << "sheet.xlsx" << "sheet.xlsx"
                               << "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"_ba
                               << "form-data; name=\"spreadsheet\"; filename=sheet.xlsx"_ba;
    QTest::newRow("xls-latin") << "spreadsheet"_L1 << "sheet.xlsx" << "szöveg.xlsx"
                               << "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"_ba
                               << "form-data; name=\"spreadsheet\"; filename*=ISO-8859-1''sz%F6veg.xlsx"_ba;
    QTest::newRow("xls-unicode") << "spreadsheet"_L1 << "sheet.xlsx" << "テキスト.xlsx"
                                 << "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"_ba
                                 << "form-data; name=\"spreadsheet\"; filename*=UTF-8''%E3%83%86%E3%82%AD%E3%82%B9%E3%83%88.xlsx"_ba;

}

void tst_QFormDataBuilder::generateQHttpPartWithDevice()
{
    QFETCH(const QLatin1StringView, name_data);
    QFETCH(const QString, real_file_name);
    QFETCH(const QString, body_name_data);
    QFETCH(const QByteArray, expected_content_type_data);
    QFETCH(const QByteArray, expected_content_disposition_data);

    QString testData = QFileInfo(QFINDTESTDATA(real_file_name)).absoluteFilePath();
    QFile data_file(testData);

    QHttpPart httpPart = QFormDataPartBuilder(name_data, QFormDataPartBuilder::PrivateConstructor())
                        .setBodyDevice(&data_file, body_name_data)
                        .build();

    QByteArray msg;
    {
        QBuffer buf(&msg);
        QVERIFY(buf.open(QIODevice::WriteOnly));
        QDebug debug(&buf);
        debug << httpPart;
    }

    QVERIFY(msg.contains(expected_content_type_data));
    QVERIFY(msg.contains(expected_content_disposition_data));
}

void tst_QFormDataBuilder::escapesBackslashAndQuotesInFilenameAndName_data()
{
    QTest::addColumn<QLatin1StringView>("name_data");
    QTest::addColumn<QString>("body_name_data");
    QTest::addColumn<QByteArray>("expected_content_type_data");
    QTest::addColumn<QByteArray>("expected_content_disposition_data");

    QTest::newRow("quote") << "t\"ext"_L1 << "rfc3252.txt" << "text/plain"_ba
                           << R"(form-data; name="t\"ext"; filename=rfc3252.txt)"_ba;

    QTest::newRow("slash") << "t\\ext"_L1 << "rfc3252.txt" << "text/plain"_ba
                           << R"(form-data; name="t\\ext"; filename=rfc3252.txt)"_ba;

    QTest::newRow("quotes") << "t\"e\"xt"_L1 << "rfc3252.txt" << "text/plain"_ba
                            << R"(form-data; name="t\"e\"xt"; filename=rfc3252.txt)"_ba;

    QTest::newRow("slashes") << "t\\\\ext"_L1 << "rfc3252.txt" << "text/plain"_ba
                             << R"(form-data; name="t\\\\ext"; filename=rfc3252.txt)"_ba;

    QTest::newRow("quote-slash") << "t\"ex\\t"_L1 << "rfc3252.txt" << "text/plain"_ba
                                 << R"(form-data; name="t\"ex\\t"; filename=rfc3252.txt)"_ba;

    QTest::newRow("quotes-slashes") << "t\"e\"x\\t\\"_L1 << "rfc3252.txt" << "text/plain"_ba
                                    << R"(form-data; name="t\"e\"x\\t\\"; filename=rfc3252.txt)"_ba;
}

void tst_QFormDataBuilder::escapesBackslashAndQuotesInFilenameAndName()
{
    QFETCH(const QLatin1StringView, name_data);
    QFETCH(const QString, body_name_data);
    QFETCH(const QByteArray, expected_content_type_data);
    QFETCH(const QByteArray, expected_content_disposition_data);

    QFile dummy_file(body_name_data);

    QHttpPart httpPart = QFormDataPartBuilder(name_data, QFormDataPartBuilder::PrivateConstructor())
                        .setBodyDevice(&dummy_file, body_name_data)
                        .build();

    QByteArray msg;
    {
        QBuffer buf(&msg);
        QVERIFY(buf.open(QIODevice::WriteOnly));
        QDebug debug(&buf);
        debug << httpPart;
    }

    QVERIFY(msg.contains(expected_content_type_data));
    QVERIFY(msg.contains(expected_content_disposition_data));
}

void tst_QFormDataBuilder::picksUtf8EncodingOnlyIfL1OrAsciiDontSuffice_data()
{
    QTest::addColumn<QLatin1StringView>("name_data");
    QTest::addColumn<QAnyStringView>("body_name_data");
    QTest::addColumn<QByteArray>("expected_content_type_data");
    QTest::addColumn<QByteArray>("expected_content_disposition_data");

    QTest::newRow("latin1-ascii") << "text"_L1 << QAnyStringView("rfc3252.txt"_L1) << "text/plain"_ba
                                  << "form-data; name=\"text\"; filename=rfc3252.txt"_ba;
    QTest::newRow("u8-ascii") << "text"_L1 << QAnyStringView(u8"rfc3252.txt") << "text/plain"_ba
                              << "form-data; name=\"text\"; filename=rfc3252.txt"_ba;
    QTest::newRow("u-ascii") << "text"_L1 << QAnyStringView(u"rfc3252.txt") << "text/plain"_ba
                             << "form-data; name=\"text\"; filename=rfc3252.txt"_ba;


    QTest::newRow("latin1-latin") << "text"_L1 << QAnyStringView("sz\366veg.txt"_L1) << "text/plain"_ba
                                  << "form-data; name=\"text\"; filename*=ISO-8859-1''sz%F6veg.txt"_ba;
    QTest::newRow("u8-latin") << "text"_L1 << QAnyStringView(u8"szöveg.txt") << "text/plain"_ba
                              << "form-data; name=\"text\"; filename*=ISO-8859-1''sz%F6veg.txt"_ba;
    QTest::newRow("u-latin") << "text"_L1 << QAnyStringView(u"szöveg.txt") << "text/plain"_ba
                             << "form-data; name=\"text\"; filename*=ISO-8859-1''sz%F6veg.txt"_ba;

    QTest::newRow("u8-u8") << "text"_L1 << QAnyStringView(u8"テキスト.txt") << "text/plain"_ba
                           << "form-data; name=\"text\"; filename*=UTF-8''%E3%83%86%E3%82%AD%E3%82%B9%E3%83%88.txt"_ba;
}

void tst_QFormDataBuilder::picksUtf8EncodingOnlyIfL1OrAsciiDontSuffice()
{
    QFETCH(const QLatin1StringView, name_data);
    QFETCH(const QAnyStringView, body_name_data);
    QFETCH(const QByteArray, expected_content_type_data);
    QFETCH(const QByteArray, expected_content_disposition_data);

    QBuffer buff;

    QHttpPart httpPart = QFormDataPartBuilder(name_data, QFormDataPartBuilder::PrivateConstructor())
                        .setBodyDevice(&buff, body_name_data)
                        .build();

    QByteArray msg;
    {
        QBuffer buf(&msg);
        QVERIFY(buf.open(QIODevice::WriteOnly));
        QDebug debug(&buf);
        debug << httpPart;
    }

    QVERIFY(msg.contains(expected_content_type_data));
    QVERIFY(msg.contains(expected_content_disposition_data));
}


QTEST_MAIN(tst_QFormDataBuilder)
#include "tst_qformdatabuilder.moc"
