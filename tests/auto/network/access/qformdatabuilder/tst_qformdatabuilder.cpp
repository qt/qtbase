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

    void setHeadersDoesNotAffectHeaderFieldsManagedByBuilder_data();
    void setHeadersDoesNotAffectHeaderFieldsManagedByBuilder();

    void specifyMimeType_data();
    void specifyMimeType();
};

void tst_QFormDataBuilder::generateQHttpPartWithDevice_data()
{
    QTest::addColumn<QLatin1StringView>("name_data");
    QTest::addColumn<QString>("real_file_name");
    QTest::addColumn<QString>("body_name_data");
    QTest::addColumn<QString>("expected_content_type_data");
    QTest::addColumn<QString>("expected_content_disposition_data");

    QTest::newRow("txt-ascii") << "text"_L1 << u"rfc3252.txt"_s << u"rfc3252.txt"_s << u"text/plain"_s
                               << uR"(form-data; name="text"; filename=rfc3252.txt)"_s;
    QTest::newRow("txt-latin") << "text"_L1 << u"rfc3252.txt"_s << u"szöveg.txt"_s << u"text/plain"_s
                               << uR"(form-data; name="text"; filename*=ISO-8859-1''sz%F6veg.txt)"_s;
    QTest::newRow("txt-unicode") << "text"_L1 << u"rfc3252.txt"_s << u"テキスト.txt"_s << u"text/plain"_s
                                 << uR"(form-data; name="text"; filename*=UTF-8''%E3%83%86%E3%82%AD%E3%82%B9%E3%83%88.txt)"_s;

    QTest::newRow("jpg-ascii") << "image"_L1 << u"image1.jpg"_s << u"image1.jpg"_s << u"image/jpeg"_s
                               << uR"(form-data; name="image"; filename=image1.jpg)"_s;
    QTest::newRow("jpg-latin") << "image"_L1 << u"image1.jpg"_s << u"kép.jpg"_s << u"image/jpeg"_s
                               << uR"(form-data; name="image"; filename*=ISO-8859-1''k%E9p.jpg)"_s;
    QTest::newRow("jpg-unicode") << "image"_L1 << u"image1.jpg"_s << u"絵.jpg"_s << u"image/jpeg"_s
                                 << uR"(form-data; name="image"; filename*=UTF-8''%E7%B5%B5)"_s;

    QTest::newRow("doc-ascii") << "text"_L1 << u"document.docx"_s << u"word.docx"_s
                               << u"application/vnd.openxmlformats-officedocument.wordprocessingml.document"_s
                               << uR"(form-data; name="text"; filename=word.docx)"_s;
    QTest::newRow("doc-latin") << "text"_L1 << u"document.docx"_s << u"szöveg.docx"_s
                               << u"application/vnd.openxmlformats-officedocument.wordprocessingml.document"_s
                               << uR"(form-data; name="text"; filename*=ISO-8859-1''sz%F6veg.docx)"_s;
    QTest::newRow("doc-unicode") << "text"_L1 << u"document.docx"_s << u"テキスト.docx"_s
                                 << u"application/vnd.openxmlformats-officedocument.wordprocessingml.document"_s
                                 << uR"(form-data; name="text"; filename*=UTF-8''%E3%83%86%E3%82%AD%E3%82%B9%E3%83%88.docx)"_s;

    QTest::newRow("xls-ascii") << "spreadsheet"_L1 << u"sheet.xlsx"_s << u"sheet.xlsx"_s
                               << u"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"_s
                               << uR"(form-data; name="spreadsheet"; filename=sheet.xlsx)"_s;
    QTest::newRow("xls-latin") << "spreadsheet"_L1 << u"sheet.xlsx"_s << u"szöveg.xlsx"_s
                               << u"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"_s
                               << uR"(form-data; name="spreadsheet"; filename*=ISO-8859-1''sz%F6veg.xlsx)"_s;
    QTest::newRow("xls-unicode") << "spreadsheet"_L1 << u"sheet.xlsx"_s << u"テキスト.xlsx"_s
                                 << u"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"_s
                                 << uR"(form-data; name="spreadsheet"; filename*=UTF-8''%E3%83%86%E3%82%AD%E3%82%B9%E3%83%88.xlsx)"_s;
}

void tst_QFormDataBuilder::generateQHttpPartWithDevice()
{
    QFETCH(const QLatin1StringView, name_data);
    QFETCH(const QString, real_file_name);
    QFETCH(const QString, body_name_data);
    QFETCH(const QString, expected_content_type_data);
    QFETCH(const QString, expected_content_disposition_data);

    QString testData = QFileInfo(QFINDTESTDATA(real_file_name)).absoluteFilePath();
    QFile data_file(testData);

    QFormDataBuilder qfdb;
    QFormDataPartBuilder &qfdpb = qfdb.part(name_data).setBodyDevice(&data_file, body_name_data);
    const QHttpPart httpPart = qfdpb.build();

    const auto msg = QDebug::toString(httpPart);
    QVERIFY(msg.contains(expected_content_type_data));
    QVERIFY(msg.contains(expected_content_disposition_data));
}

void tst_QFormDataBuilder::escapesBackslashAndQuotesInFilenameAndName_data()
{
    QTest::addColumn<QLatin1StringView>("name_data");
    QTest::addColumn<QString>("body_name_data");
    QTest::addColumn<QString>("expected_content_type_data");
    QTest::addColumn<QString>("expected_content_disposition_data");

    QTest::newRow("quote") << "t\"ext"_L1 << "rfc3252.txt" << u"text/plain"_s
                           << uR"(form-data; name="t\"ext"; filename=rfc3252.txt)"_s;

    QTest::newRow("slash") << "t\\ext"_L1 << "rfc3252.txt" << u"text/plain"_s
                           << uR"(form-data; name="t\\ext"; filename=rfc3252.txt)"_s;

    QTest::newRow("quotes") << "t\"e\"xt"_L1 << "rfc3252.txt" << u"text/plain"_s
                            << uR"(form-data; name="t\"e\"xt"; filename=rfc3252.txt)"_s;

    QTest::newRow("slashes") << "t\\\\ext"_L1 << "rfc3252.txt" << u"text/plain"_s
                             << uR"(form-data; name="t\\\\ext"; filename=rfc3252.txt)"_s;

    QTest::newRow("quote-slash") << "t\"ex\\t"_L1 << "rfc3252.txt" << u"text/plain"_s
                                 << uR"(form-data; name="t\"ex\\t"; filename=rfc3252.txt)"_s;

    QTest::newRow("quotes-slashes") << "t\"e\"x\\t\\"_L1 << "rfc3252.txt" << u"text/plain"_s
                                    << uR"(form-data; name="t\"e\"x\\t\\"; filename=rfc3252.txt)"_s;
}

void tst_QFormDataBuilder::escapesBackslashAndQuotesInFilenameAndName()
{
    QFETCH(const QLatin1StringView, name_data);
    QFETCH(const QString, body_name_data);
    QFETCH(const QString, expected_content_type_data);
    QFETCH(const QString, expected_content_disposition_data);

    QFile dummy_file(body_name_data);

    QFormDataBuilder qfdb;
    QFormDataPartBuilder &qfdpb = qfdb.part(name_data).setBodyDevice(&dummy_file, body_name_data);
    const QHttpPart httpPart = qfdpb.build();

    const auto msg = QDebug::toString(httpPart);
    QVERIFY(msg.contains(expected_content_type_data));
    QVERIFY(msg.contains(expected_content_disposition_data));
}

void tst_QFormDataBuilder::picksUtf8EncodingOnlyIfL1OrAsciiDontSuffice_data()
{
    QTest::addColumn<QLatin1StringView>("name_data");
    QTest::addColumn<QAnyStringView>("body_name_data");
    QTest::addColumn<QString>("expected_content_type_data");
    QTest::addColumn<QString>("expected_content_disposition_data");

    QTest::newRow("latin1-ascii") << "text"_L1 << QAnyStringView("rfc3252.txt"_L1) << u"text/plain"_s
                                  << uR"(form-data; name="text"; filename=rfc3252.txt)"_s;
    QTest::newRow("u8-ascii") << "text"_L1 << QAnyStringView(u8"rfc3252.txt") << u"text/plain"_s
                              << uR"(form-data; name="text"; filename=rfc3252.txt)"_s;
    QTest::newRow("u-ascii") << "text"_L1 << QAnyStringView(u"rfc3252.txt") << u"text/plain"_s
                             << uR"(form-data; name="text"; filename=rfc3252.txt)"_s;

    QTest::newRow("latin1-latin") << "text"_L1 << QAnyStringView("sz\366veg.txt"_L1) << u"text/plain"_s
                                  << uR"(form-data; name="text"; filename*=ISO-8859-1''sz%F6veg.txt)"_s;
    QTest::newRow("u8-latin") << "text"_L1 << QAnyStringView(u8"szöveg.txt") << u"text/plain"_s
                              << uR"(form-data; name="text"; filename*=ISO-8859-1''sz%F6veg.txt)"_s;
    QTest::newRow("u-latin") << "text"_L1 << QAnyStringView(u"szöveg.txt") << u"text/plain"_s
                             << uR"(form-data; name="text"; filename*=ISO-8859-1''sz%F6veg.txt)"_s;

    QTest::newRow("u8-u8") << "text"_L1 << QAnyStringView(u8"テキスト.txt") << u"text/plain"_s
                           << uR"(form-data; name="text"; filename*=UTF-8''%E3%83%86%E3%82%AD%E3%82%B9%E3%83%88.txt)"_s;
}

void tst_QFormDataBuilder::picksUtf8EncodingOnlyIfL1OrAsciiDontSuffice()
{
    QFETCH(const QLatin1StringView, name_data);
    QFETCH(const QAnyStringView, body_name_data);
    QFETCH(const QString, expected_content_type_data);
    QFETCH(const QString, expected_content_disposition_data);

    QBuffer buff;

    QFormDataBuilder qfdb;
    QFormDataPartBuilder &qfdpb = qfdb.part(name_data).setBodyDevice(&buff, body_name_data);
    const QHttpPart httpPart = qfdpb.build();

    const auto msg = QDebug::toString(httpPart);
    QVERIFY(msg.contains(expected_content_type_data));
    QVERIFY(msg.contains(expected_content_disposition_data));
}

void tst_QFormDataBuilder::setHeadersDoesNotAffectHeaderFieldsManagedByBuilder_data()
{
    QTest::addColumn<QLatin1StringView>("name_data");
    QTest::addColumn<QAnyStringView>("body_name_data");
    QTest::addColumn<bool>("overwrite");
    QTest::addColumn<bool>("extra_headers");
    QTest::addColumn<QStringList>("expected_headers");

    QTest::newRow("content-disposition-is-set-by-default")
            << "text"_L1 << QAnyStringView("rfc3252.txt"_L1)
            << false << false
            << QStringList{
                uR"("content-disposition":"form-data; name=\"text\"; filename=rfc3252.txt")"_s,
                uR"("content-type":"text/plain")"_s};

    QTest::newRow("default-overwrites-preset-content-disposition")
            << "text"_L1 << QAnyStringView("rfc3252.txt"_L1)
            << true << false
            << QStringList{
                uR"("content-disposition":"form-data; name=\"text\"; filename=rfc3252.txt")"_s,
                uR"("content-type":"text/plain")"_s};

    QTest::newRow("added-extra-header")
            << "text"_L1 << QAnyStringView("rfc3252.txt"_L1)
            << false << true
            << QStringList{
                uR"("content-disposition":"form-data; name=\"text\"; filename=rfc3252.txt")"_s,
                uR"("content-type":"text/plain")"_s,
                uR"("content-length":"70")"_s};

    QTest::newRow("extra-header-and-overwrite")
            << "text"_L1 << QAnyStringView("rfc3252.txt"_L1)
            << true << true
            << QStringList{
                uR"("content-disposition":"form-data; name=\"text\"; filename=rfc3252.txt")"_s,
                uR"("content-type":"text/plain")"_s,
                uR"("content-length":"70")"_s};
}

void tst_QFormDataBuilder::setHeadersDoesNotAffectHeaderFieldsManagedByBuilder()
{
    QFETCH(const QLatin1StringView, name_data);
    QFETCH(const QAnyStringView, body_name_data);
    QFETCH(const bool, overwrite);
    QFETCH(const bool, extra_headers);
    QFETCH(const QStringList, expected_headers);

    QBuffer buff;

    QFormDataBuilder qfdb;
    QFormDataPartBuilder &qfdpb = qfdb.part(name_data).setBodyDevice(&buff, body_name_data);

    if (overwrite || extra_headers) {
        QHttpHeaders headers;

        if (overwrite) {
            headers.append(QHttpHeaders::WellKnownHeader::ContentType, "attachment");
            qfdpb.setHeaders(headers);
        }

        if (extra_headers) {
            headers.append(QHttpHeaders::WellKnownHeader::ContentLength, "70");
            qfdpb.setHeaders(std::move(headers));
        }
    }

    const QHttpPart httpPart = qfdpb.build();

    const auto msg = QDebug::toString(httpPart);
    for (const auto &header : expected_headers)
        QVERIFY2(msg.contains(header), qPrintable(header));
}

void tst_QFormDataBuilder::specifyMimeType_data()
{
    QTest::addColumn<QLatin1StringView>("name_data");
    QTest::addColumn<QAnyStringView>("body_name_data");
    QTest::addColumn<QAnyStringView>("mime_type");
    QTest::addColumn<QString>("expected_content_type_data");

    QTest::newRow("not-specified") << "text"_L1 << QAnyStringView("rfc3252.txt"_L1)
        << QAnyStringView("text/plain"_L1) << uR"("content-type":"text/plain")"_s;
    QTest::newRow("mime-specified") << "text"_L1 << QAnyStringView("rfc3252.txt"_L1)
        << QAnyStringView("text/plain"_L1) << uR"("content-type":"text/plain")"_s;
    // wrong mime type specified but it is not overridden by the deduction
    QTest::newRow("wrong-mime-specified") << "text"_L1 << QAnyStringView("rfc3252.txt"_L1)
        << QAnyStringView("image/jpeg"_L1) << uR"("content-type":"image/jpeg)"_s;
}

void tst_QFormDataBuilder::specifyMimeType()
{
    QFETCH(const QLatin1StringView, name_data);
    QFETCH(const QAnyStringView, body_name_data);
    QFETCH(const QAnyStringView, mime_type);
    QFETCH(const QString, expected_content_type_data);

    QBuffer buff;

    QFormDataBuilder qfdb;
    QFormDataPartBuilder &qfdpb = qfdb.part(name_data).setBodyDevice(&buff, body_name_data);

    if (!mime_type.empty())
        qfdpb.setBodyDevice(&buff, body_name_data, mime_type);
    else
        qfdpb.setBodyDevice(&buff, body_name_data);

    const QHttpPart httpPart = qfdpb.build();

    const auto msg = QDebug::toString(httpPart);
    QVERIFY(msg.contains(expected_content_type_data));
}


QTEST_MAIN(tst_QFormDataBuilder)
#include "tst_qformdatabuilder.moc"
