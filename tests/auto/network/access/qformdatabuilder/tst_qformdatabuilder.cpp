// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtNetwork/private/qhttpmultipart_p.h>
#include <QtNetwork/qformdatabuilder.h>

#include <QtCore/qbuffer.h>
#include <QtCore/qfile.h>

#include <QtTest/qtest.h>

#ifndef QTEST_THROW_ON_FAIL
# error This test requires QTEST_THROW_ON_FAIL being active.
#endif

#ifndef QTEST_THROW_ON_SKIP
# error This test requires QTEST_THROW_ON_SKIP being active.
#endif

#include <QtCore/qxpfunctional.h>
#include <string>
#include <type_traits>

using namespace Qt::StringLiterals;

const auto CRLF = "\r\n"_ba;

Q_NEVER_INLINE static QByteArray
serialized_impl([[maybe_unused]] qxp::function_ref<QFormDataBuilder &(QFormDataBuilder &)> operations,
                QFormDataBuilder::Options options = QFormDataBuilder::Option::Default)
{
#if defined(QT_UNDEFINED_SANITIZER) && !defined(QT_BUILD_INTERNAL)
    QSKIP("This test requires -developer-build when --sanitize=undefined is active.");
#else
    QFormDataBuilder builder;

    const std::unique_ptr<QHttpMultiPart> mp = operations(builder).buildMultiPart(options);

    auto *device = QHttpMultiPartPrivate::get(mp.get())->device;
    QVERIFY(device->open(QIODeviceBase::ReadOnly));
    return device->readAll();
#endif // QT_BUILD_INTERNAL || !QT_UNDEFINED_SANITIZER
}

template <typename Callable>
static QByteArray serialized(Callable operation,
                             QFormDataBuilder::Options options = QFormDataBuilder::Option::Default)
{
    if constexpr (std::is_void_v<std::invoke_result_t<Callable&, QFormDataBuilder&>>) {
        return serialized_impl([&](auto &builder) {
               operation(builder);
               return std::ref(builder);
            }, options);
    } else {
        return serialized_impl(std::move(operation));
    }
}

class tst_QFormDataBuilder : public QObject
{
    Q_OBJECT

    void checkBodyPartsAreEquivalent(QByteArrayView expected, QByteArrayView actual);

private Q_SLOTS:
    void generateQHttpPartWithDevice_data();
    void generateQHttpPartWithDevice();

    void escapesBackslashAndQuotesInFilenameAndName_data();
    void escapesBackslashAndQuotesInFilenameAndName();

    void filenameEncoding_data();
    void filenameEncoding();

    void setHeadersDoesNotAffectHeaderFieldsManagedByBuilder_data();
    void setHeadersDoesNotAffectHeaderFieldsManagedByBuilder();

    void specifyMimeType_data();
    void specifyMimeType();

    void picksUtf8NameEncodingIfAsciiDoesNotSuffice_data();
    void picksUtf8NameEncodingIfAsciiDoesNotSuffice();

    void moveSemantics();
    void keepResultOfCallingPartAliveAmongSubsequentCallsToPart();
};

void tst_QFormDataBuilder::checkBodyPartsAreEquivalent(QByteArrayView expected, QByteArrayView actual)
{
    qsizetype expectedCrlfPos = expected.indexOf(CRLF);
    qsizetype expectedBoundaryPos = expected.lastIndexOf("--boundary_.oOo.");

    qsizetype actualCrlfPos = actual.indexOf(CRLF);
    qsizetype actualBoundaryPos = actual.lastIndexOf("--boundary_.oOo.");

    qsizetype start = expectedCrlfPos + 2;
    qsizetype end = expectedBoundaryPos - expectedCrlfPos - 2;

    QCOMPARE(actualCrlfPos, expectedCrlfPos);
    QCOMPARE(actualBoundaryPos, expectedBoundaryPos);
    QCOMPARE(actual.sliced(start, end), expected.sliced(start, end));
}

void tst_QFormDataBuilder::generateQHttpPartWithDevice_data()
{
    QTest::addColumn<QLatin1StringView>("name_data");
    QTest::addColumn<QString>("real_file_name");
    QTest::addColumn<QString>("body_name_data");
    QTest::addColumn<QByteArray>("expected_content_type_data");
    QTest::addColumn<QByteArray>("expected_content_disposition_data");
    QTest::addColumn<QByteArray>("content_disposition_must_not_contain_data");

    QTest::newRow("txt-ascii") << "text"_L1 << u"rfc3252.txt"_s << u"rfc3252.txt"_s << "text/plain"_ba
                               << R"(form-data; name="text"; filename="rfc3252.txt")"_ba
                               << "filename*"_ba;
    QTest::newRow("txt-latin") << "text"_L1 << u"rfc3252.txt"_s << u"szöveg.txt"_s << "text/plain"_ba
                               << R"(form-data; name="text"; filename="szöveg.txt"; filename*=UTF-8''sz%C3%B6veg.txt)"_ba
                               << ""_ba;
    QTest::newRow("txt-unicode") << "text"_L1 << u"rfc3252.txt"_s << u"テキスト.txt"_s << "text/plain"_ba
                                 << R"(form-data; name="text"; filename="テキスト.txt"; filename*=UTF-8''%E3%83%86%E3%82%AD%E3%82%B9%E3%83%88.txt)"_ba
                                 << ""_ba;

    QTest::newRow("jpg-ascii") << "image"_L1 << u"image1.jpg"_s << u"image1.jpg"_s << "image/jpeg"_ba
                               << R"(form-data; name="image"; filename="image1.jpg")"_ba
                               << "filename*"_ba;
    QTest::newRow("jpg-latin") << "image"_L1 << u"image1.jpg"_s << u"kép.jpg"_s << "image/jpeg"_ba
                               << R"(form-data; name="image"; filename="kép.jpg"; filename*=UTF-8''k%C3%A9p.jpg)"_ba
                               << ""_ba;
    QTest::newRow("jpg-unicode") << "image"_L1 << u"image1.jpg"_s << u"絵.jpg"_s << "image/jpeg"_ba
                                 << R"(form-data; name="image"; filename="絵.jpg"; filename*=UTF-8''%E7%B5%B5.jpg)"_ba
                                 << ""_ba;

    QTest::newRow("doc-ascii") << "text"_L1 << u"document.docx"_s << u"word.docx"_s
                               << "application/vnd.openxmlformats-officedocument.wordprocessingml.document"_ba
                               << R"(form-data; name="text"; filename="word.docx")"_ba
                               << "filename*"_ba;
    QTest::newRow("doc-latin") << "text"_L1 << u"document.docx"_s << u"szöveg.docx"_s
                               << "application/vnd.openxmlformats-officedocument.wordprocessingml.document"_ba
                               << R"(form-data; name="text"; filename="szöveg.docx"; filename*=UTF-8''sz%C3%B6veg.docx)"_ba
                               << ""_ba;
    QTest::newRow("doc-unicode") << "text"_L1 << u"document.docx"_s << u"テキスト.docx"_s
                                 << "application/vnd.openxmlformats-officedocument.wordprocessingml.document"_ba
                                 << R"(form-data; name="text"; filename="テキスト.docx"; filename*=UTF-8''%E3%83%86%E3%82%AD%E3%82%B9%E3%83%88.docx)"_ba
                                 << ""_ba;

    QTest::newRow("xls-ascii") << "spreadsheet"_L1 << u"sheet.xlsx"_s << u"sheet.xlsx"_s
                               << "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"_ba
                               << R"(form-data; name="spreadsheet"; filename="sheet.xlsx")"_ba
                               << "filename*"_ba;
    QTest::newRow("xls-latin") << "spreadsheet"_L1 << u"sheet.xlsx"_s << u"szöveg.xlsx"_s
                               << "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"_ba
                               << R"(form-data; name="spreadsheet"; filename="szöveg.xlsx"; filename*=UTF-8''sz%C3%B6veg.xlsx)"_ba
                               << ""_ba;
    QTest::newRow("xls-unicode") << "spreadsheet"_L1 << u"sheet.xlsx"_s << u"テキスト.xlsx"_s
                                 << "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"_ba
                                 << R"(form-data; name="spreadsheet"; filename="テキスト.xlsx"; filename*=UTF-8''%E3%83%86%E3%82%AD%E3%82%B9%E3%83%88.xlsx)"_ba
                                 << ""_ba;
}

void tst_QFormDataBuilder::generateQHttpPartWithDevice()
{
    QFETCH(const QLatin1StringView, name_data);
    QFETCH(const QString, real_file_name);
    QFETCH(const QString, body_name_data);
    QFETCH(const QByteArray, expected_content_type_data);
    QFETCH(const QByteArray, expected_content_disposition_data);
    QFETCH(const QByteArray, content_disposition_must_not_contain_data);

    QString testData = QFileInfo(QFINDTESTDATA(real_file_name)).absoluteFilePath();
    QFile data_file(testData);
    QVERIFY2(data_file.open(QIODevice::ReadOnly), qPrintable(data_file.errorString()));

    const auto msg = serialized([&](auto &builder) {
            builder.part(name_data).setBodyDevice(&data_file, body_name_data);
        });

    QVERIFY(msg.contains(expected_content_type_data));
    QVERIFY(msg.contains(expected_content_disposition_data));
    if (!content_disposition_must_not_contain_data.isEmpty())
        QVERIFY(!msg.contains(content_disposition_must_not_contain_data));
}

void tst_QFormDataBuilder::escapesBackslashAndQuotesInFilenameAndName_data()
{
    QTest::addColumn<QLatin1StringView>("name_data");
    QTest::addColumn<QString>("body_name_data");
    QTest::addColumn<QByteArray>("expected_content_type_data");
    QTest::addColumn<QByteArray>("expected_content_disposition_data");

    QTest::newRow("quote") << "t\"ext"_L1 << uR"(rfc32"52.txt)"_s << "text/plain"_ba
                           << R"(form-data; name="t\"ext"; filename="rfc32\"52.txt")"_ba;

    QTest::newRow("slash") << "t\\ext"_L1 << uR"(rfc32\52.txt)"_s << "text/plain"_ba
                           << R"(form-data; name="t\\ext"; filename="rfc32\\52.txt")"_ba;

    QTest::newRow("quotes") << "t\"e\"xt"_L1 << uR"(rfc3"25"2.txt)"_s << "text/plain"_ba
                            << R"(form-data; name="t\"e\"xt"; filename="rfc3\"25\"2.txt")"_ba;

    QTest::newRow("slashes") << "t\\\\ext"_L1 << uR"(rfc32\\52.txt)"_s << "text/plain"_ba
                             << R"(form-data; name="t\\\\ext"; filename="rfc32\\\\52.txt")"_ba;

    QTest::newRow("quote-slash") << "t\"ex\\t"_L1 << uR"(rfc"32\52.txt)"_s << "text/plain"_ba
                                 << R"(form-data; name="t\"ex\\t"; filename="rfc\"32\\52.txt")"_ba;

    QTest::newRow("quotes-slashes") << "t\"e\"x\\t\\"_L1 << uR"(r"f"c3\2\52.txt)"_s << "text/plain"_ba
                                    << R"(form-data; name="t\"e\"x\\t\\"; filename="r\"f\"c3\\2\\52.txt")"_ba;
}

void tst_QFormDataBuilder::escapesBackslashAndQuotesInFilenameAndName()
{
    QFETCH(const QLatin1StringView, name_data);
    QFETCH(const QString, body_name_data);
    QFETCH(const QByteArray, expected_content_type_data);
    QFETCH(const QByteArray, expected_content_disposition_data);

    QBuffer dummy_file;
    QVERIFY(dummy_file.open(QIODevice::ReadOnly));

    const auto msg = serialized([&](auto &builder) {
            builder.part(name_data).setBodyDevice(&dummy_file, body_name_data);
        });

    QVERIFY(msg.contains(expected_content_type_data));
    QVERIFY(msg.contains(expected_content_disposition_data));
}

void tst_QFormDataBuilder::filenameEncoding_data()
{
    static const auto contentType = "text/plain"_ba;
    using Opts = QFormDataBuilder::Options;
    using Opt = QFormDataBuilder::Option;
    QTest::addColumn<QLatin1StringView>("name_data");
    QTest::addColumn<QAnyStringView>("body_name_data");
    QTest::addColumn<QByteArray>("expected_content_type_data");
    QTest::addColumn<QByteArray>("expected_content_disposition_data");
    QTest::addColumn<QByteArray>("content_disposition_must_not_contain_data");
    QTest::addColumn<QFormDataBuilder::Options>("filename_options");

    auto addAsciiTestRows = [] (const std::string &rowName, Opts opts) {
        QTest::newRow((rowName + "-L1").c_str())
            << "text"_L1 << QAnyStringView("rfc3252.txt"_L1) << contentType
            << R"(form-data; name="text"; filename="rfc3252.txt")"_ba
            << "filename*"_ba << opts;
        QTest::newRow((rowName + "-U8").c_str())
            << "text"_L1 << QAnyStringView(u8"rfc3252.txt") << contentType
            << R"(form-data; name="text"; filename="rfc3252.txt")"_ba
            << "filename*"_ba << opts;
        QTest::newRow((rowName + "-U").c_str())
            << "text"_L1 << QAnyStringView(u"rfc3252.txt") << contentType
            << R"(form-data; name="text"; filename="rfc3252.txt")"_ba
            << "filename*"_ba << opts;
    };
    addAsciiTestRows("default-ascii", Opt::Default);
    addAsciiTestRows("omit-rfc8187-ascii", Opt::OmitRfc8187EncodedFilename);
    addAsciiTestRows("use-rfc7578-ascii", Opt::UseRfc7578PercentEncodedFilename);
    addAsciiTestRows("strict-rfc7578-ascii", Opt::StrictRfc7578);
    addAsciiTestRows("prefer-latin1-ascii", Opt::PreferLatin1EncodedFilename);

    auto addLatin1TestRows = [] (const std::string &rowName, const QByteArray &resultFilename,
                                 const QByteArray &mustNotContain, Opts opts) {
        // 0xF6 is 'ö', use hex value with Latin-1 to avoid interpretation as UTF-8 (0xC3 0xB6)
        QTest::newRow((rowName + "-L1").c_str())
            << "text"_L1 << QAnyStringView("sz\xF6veg.txt"_L1) << contentType
            << resultFilename << mustNotContain << opts;
        QTest::newRow((rowName + "-U8").c_str())
            << "text"_L1 << QAnyStringView(u8"szöveg.txt") << contentType
            << resultFilename << mustNotContain << opts;
        QTest::newRow((rowName + "-U").c_str())
            << "text"_L1 << QAnyStringView(u"szöveg.txt") << contentType
            << resultFilename << mustNotContain << opts;
    };
    addLatin1TestRows("default-latin1",
                      R"(form-data; name="text"; filename="szöveg.txt"; filename*=UTF-8''sz%C3%B6veg.txt)"_ba,
                      ""_ba, Opt::Default);
    addLatin1TestRows("omit-rfc8187-latin1",
                      R"(form-data; name="text"; filename="szöveg.txt")"_ba,
                      "filename*"_ba, Opt::OmitRfc8187EncodedFilename);
    addLatin1TestRows("use-rfc7578-latin1",
                      R"(form-data; name="text"; filename="sz%C3%B6veg.txt"; filename*=UTF-8''sz%C3%B6veg.txt)"_ba,
                      ""_ba, Opt::UseRfc7578PercentEncodedFilename);
    addLatin1TestRows("strict-rfc7578-latin1",
                      R"(form-data; name="text"; filename="sz%C3%B6veg.txt")"_ba,
                      "filename*"_ba, Opt::StrictRfc7578);
    addLatin1TestRows("prefer-latin1-latin1",
                      "form-data; name=\"text\"; filename=\"sz\xF6veg.txt\"; filename*=ISO-8859-1''sz%F6veg.txt"_ba,
                      ""_ba, Opt::PreferLatin1EncodedFilename);

    auto addUtf8TestRows = [] (const std::string &rowName, const QByteArray &resultFilename,
                               const QByteArray &mustNotContain, Opts opts) {
        QTest::newRow((rowName + "-U8").c_str())
            << "text"_L1 << QAnyStringView(u8"テキスト.txt") << contentType
            << resultFilename << mustNotContain << opts;
    };
    addUtf8TestRows("default-utf8",
                    R"(form-data; name="text"; filename="テキスト.txt"; filename*=UTF-8''%E3%83%86%E3%82%AD%E3%82%B9%E3%83%88.txt)"_ba,
                    ""_ba, Opt::Default);
    addUtf8TestRows("omit-rfc8187-utf8",
                    R"(form-data; name="text"; filename="テキスト.txt")"_ba,
                    "filename*"_ba, Opt::OmitRfc8187EncodedFilename);
    addUtf8TestRows("use-rfc7578-utf8",
                    R"(form-data; name="text"; filename="%E3%83%86%E3%82%AD%E3%82%B9%E3%83%88.txt"; filename*=UTF-8''%E3%83%86%E3%82%AD%E3%82%B9%E3%83%88.txt)"_ba,
                    ""_ba, Opt::UseRfc7578PercentEncodedFilename);
    addUtf8TestRows("strict-rfc7578-utf8",
                    R"(form-data; name="text"; filename="%E3%83%86%E3%82%AD%E3%82%B9%E3%83%88.txt")"_ba,
                    "filename*"_ba, Opt::StrictRfc7578);
    addUtf8TestRows("strict-rfc7578-utf8",
                    R"(form-data; name="text"; filename="%E3%83%86%E3%82%AD%E3%82%B9%E3%83%88.txt")"_ba,
                    "filename*"_ba, Opt::StrictRfc7578);
    addUtf8TestRows("prefer-latin1-utf8",
                    R"(form-data; name="text"; filename="テキスト.txt"; filename*=UTF-8''%E3%83%86%E3%82%AD%E3%82%B9%E3%83%88.txt)"_ba,
                    ""_ba, Opt::PreferLatin1EncodedFilename);
}

void tst_QFormDataBuilder::filenameEncoding()
{
    QFETCH(const QLatin1StringView, name_data);
    QFETCH(const QAnyStringView, body_name_data);
    QFETCH(const QByteArray, expected_content_type_data);
    QFETCH(const QByteArray, expected_content_disposition_data);
    QFETCH(const QByteArray, content_disposition_must_not_contain_data);
    QFETCH(const QFormDataBuilder::Options, filename_options);

    QBuffer buff;
    QVERIFY(buff.open(QIODevice::ReadOnly));

    const auto msg = serialized([&](auto &builder) {
            builder.part(name_data).setBodyDevice(&buff, body_name_data);
        }, filename_options);

    QVERIFY2(msg.contains(expected_content_type_data),
             "content-type not found : " + expected_content_type_data);
    QVERIFY2(msg.contains(expected_content_disposition_data),
             "content-disposition not found : " + expected_content_disposition_data);
    if (!content_disposition_must_not_contain_data.isEmpty()) {
        QVERIFY2(!msg.contains(content_disposition_must_not_contain_data),
                 "content-disposition contained data it shouldn't : "
                 + content_disposition_must_not_contain_data);
    }
}

void tst_QFormDataBuilder::setHeadersDoesNotAffectHeaderFieldsManagedByBuilder_data()
{
    QTest::addColumn<QLatin1StringView>("name_data");
    QTest::addColumn<QAnyStringView>("body_name_data");
    QTest::addColumn<bool>("overwrite");
    QTest::addColumn<bool>("extra_headers");
    QTest::addColumn<QByteArrayList>("expected_headers");

    QTest::newRow("content-disposition-is-set-by-default")
            << "text"_L1 << QAnyStringView("rfc3252.txt"_L1)
            << false << false
            << QList{
                    R"(content-disposition: form-data; name="text"; filename="rfc3252.txt")"_ba ,
                    "content-type: text/plain"_ba,
               };

    QTest::newRow("default-overwrites-preset-content-disposition")
            << "text"_L1 << QAnyStringView("rfc3252.txt"_L1)
            << true << false
            << QList{
                    R"(content-disposition: form-data; name="text"; filename="rfc3252.txt")"_ba ,
                    "content-type: text/plain"_ba,
               };

    QTest::newRow("added-extra-header")
            << "text"_L1 << QAnyStringView("rfc3252.txt"_L1)
            << false << true
            << QList{
                    R"(content-disposition: form-data; name="text"; filename="rfc3252.txt")"_ba ,
                    "content-type: text/plain"_ba,
                    "content-length: 70"_ba,
               };

    QTest::newRow("extra-header-and-overwrite")
            << "text"_L1 << QAnyStringView("rfc3252.txt"_L1)
            << true << true
            << QList{
                    R"(content-disposition: form-data; name="text"; filename="rfc3252.txt")"_ba ,
                    "content-type: text/plain"_ba,
                    "content-length: 70"_ba,
               };
}

void tst_QFormDataBuilder::setHeadersDoesNotAffectHeaderFieldsManagedByBuilder()
{
    QFETCH(const QLatin1StringView, name_data);
    QFETCH(const QAnyStringView, body_name_data);
    QFETCH(const bool, overwrite);
    QFETCH(const bool, extra_headers);
    QFETCH(const QByteArrayList, expected_headers);

    QBuffer buff;
    QVERIFY(buff.open(QIODevice::ReadOnly));

    const auto msg = serialized([&](auto &builder) {
            auto qfdpb = builder.part(name_data).setBodyDevice(&buff, body_name_data);

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
        });

    for (const auto &header : expected_headers)
        QVERIFY2(msg.contains(CRLF + header + CRLF), header);
}

void tst_QFormDataBuilder::specifyMimeType_data()
{
    QTest::addColumn<QLatin1StringView>("name_data");
    QTest::addColumn<QAnyStringView>("body_name_data");
    QTest::addColumn<QAnyStringView>("mime_type");
    QTest::addColumn<QByteArray>("expected_content_type_data");

    QTest::newRow("not-specified") << "text"_L1 << QAnyStringView("rfc3252.txt"_L1)
        << QAnyStringView("text/plain"_L1) << "content-type: text/plain"_ba;
    QTest::newRow("mime-specified") << "text"_L1 << QAnyStringView("rfc3252.txt"_L1)
        << QAnyStringView("text/plain"_L1) << "content-type: text/plain"_ba;
    // wrong mime type specified but it is not overridden by the deduction
    QTest::newRow("wrong-mime-specified") << "text"_L1 << QAnyStringView("rfc3252.txt"_L1)
        << QAnyStringView("image/jpeg"_L1) << "content-type: image/jpeg"_ba;
}

void tst_QFormDataBuilder::specifyMimeType()
{
    QFETCH(const QLatin1StringView, name_data);
    QFETCH(const QAnyStringView, body_name_data);
    QFETCH(const QAnyStringView, mime_type);
    QFETCH(const QByteArray, expected_content_type_data);

    QBuffer buff;
    QVERIFY(buff.open(QIODevice::ReadOnly));

    const auto msg = serialized([&](auto &builder) {
            auto qfdpb = builder.part(name_data).setBodyDevice(&buff, body_name_data);

            if (!mime_type.empty())
                qfdpb.setBodyDevice(&buff, body_name_data, mime_type);
            else
                qfdpb.setBodyDevice(&buff, body_name_data);
        });

    QVERIFY2(msg.contains(CRLF + expected_content_type_data + CRLF),
             msg + " does not contain " + expected_content_type_data);
}

void tst_QFormDataBuilder::picksUtf8NameEncodingIfAsciiDoesNotSuffice_data()
{
    QTest::addColumn<QAnyStringView>("name_data");
    QTest::addColumn<QByteArray>("expected_content_disposition_data");

    QTest::newRow("latin1-ascii") << QAnyStringView("text"_L1) << R"(form-data; name="text")"_ba;
    QTest::newRow("u8-ascii") << QAnyStringView(u8"text") << R"(form-data; name="text")"_ba;
    QTest::newRow("u-ascii") << QAnyStringView(u"text") << R"(form-data; name="text")"_ba;

    // 0xF6 is 'ö', use hex value with Latin-1 to avoid interpretation as UTF-8
    QTest::newRow("latin1-latin") << QAnyStringView("t\xF6xt"_L1) << R"(form-data; name="töxt")"_ba;
    QTest::newRow("u8-latin") << QAnyStringView(u8"töxt") << R"(form-data; name="töxt")"_ba;
    QTest::newRow("u-latin") << QAnyStringView(u"töxt") << R"(form-data; name="töxt")"_ba;

    QTest::newRow("u8-u8") << QAnyStringView(u8"テキスト") << R"(form-data; name="テキスト")"_ba;
}

void tst_QFormDataBuilder::picksUtf8NameEncodingIfAsciiDoesNotSuffice()
{
    QFETCH(const QAnyStringView, name_data);
    QFETCH(const QByteArray, expected_content_disposition_data);

    const auto msg = serialized([&](auto &builder) {
            builder.part(name_data).setBody("some"_ba);
        });

    QVERIFY2(msg.contains(expected_content_disposition_data),
             "content-disposition not found : " + expected_content_disposition_data);
}

void tst_QFormDataBuilder::moveSemantics()
{
    constexpr QByteArrayView expected = "--boundary_.oOo._4SUrZy7x9lPHMF3fbRSsE15hiWu5Sbmy\r\n"
                                        "content-type: text/plain\r\ncontent-disposition: form-data; name=\"text\"; filename=\"rfc3252.txt\"\r\n\r\n"
                                        "some text for reference\r\n"
                                        "--boundary_.oOo._4SUrZy7x9lPHMF3fbRSsE15hiWu5Sbmy--\r\n";

    const QString testData = QFileInfo(QFINDTESTDATA("rfc3252.txt")).absoluteFilePath();

    // We get the expected
    {
        QFile data_file(testData);
        QVERIFY2(data_file.open(QIODeviceBase::ReadOnly), qPrintable(data_file.errorString()));

        const QByteArray actual = serialized([&](auto &builder) {
                builder.part("text"_L1).setBodyDevice(&data_file, "rfc3252.txt");
            });

        checkBodyPartsAreEquivalent(expected, actual);
    }

    // We get the expected from a move constructed qfdb
    {
        QFile data_file(testData);
        QVERIFY2(data_file.open(QIODeviceBase::ReadOnly), qPrintable(data_file.errorString()));

        QFormDataBuilder qfdb;
        auto p = qfdb.part("text"_L1);
        const QByteArray actual = serialized([&, moved = std::move(qfdb)](auto &) mutable {
                p.setBodyDevice(&data_file, "rfc3252.txt");
                return std::ref(moved);
            });

        checkBodyPartsAreEquivalent(expected, actual);
    }

    // We get the expected from a move assigned qfdb
    {
        QFile data_file(testData);
        QVERIFY2(data_file.open(QIODeviceBase::ReadOnly), qPrintable(data_file.errorString()));

        QFormDataBuilder moved;

        const QByteArray actual = serialized([&](auto &builder) {
                builder.part("text"_L1).setBodyDevice(&data_file, "rfc3252.txt");
                return std::ref(moved = std::move(builder));
            });

        checkBodyPartsAreEquivalent(expected, actual);
    }
}

void tst_QFormDataBuilder::keepResultOfCallingPartAliveAmongSubsequentCallsToPart()
{
    QFormDataBuilder qfdb;
    auto p1 = qfdb.part("1"_L1);
    auto p2 = qfdb.part("2"_L1);
    auto p3 = qfdb.part("3"_L1);
    auto p4 = qfdb.part("4"_L1);
    auto p5 = qfdb.part("5"_L1);
    auto p6 = qfdb.part("6"_L1);
    auto p7 = qfdb.part("7"_L1);
    auto p8 = qfdb.part("8"_L1);
    auto p9 = qfdb.part("9"_L1);
    auto p10 = qfdb.part("10"_L1);
    auto p11 = qfdb.part("11"_L1);
    auto p12 = qfdb.part("12"_L1);

    QByteArray dummyData = "totally_a_text_file"_ba;

    p1.setBody(dummyData, "body1"_L1);
    p2.setBody(dummyData, "body2"_L1);
    p3.setBody(dummyData, "body3"_L1);
    p4.setBody(dummyData, "body4"_L1);
    p5.setBody(dummyData, "body5"_L1);
    p6.setBody(dummyData, "body6"_L1);
    p7.setBody(dummyData, "body7"_L1);
    p8.setBody(dummyData, "body8"_L1);
    p9.setBody(dummyData, "body9"_L1);
    p10.setBody(dummyData, "body10"_L1);
    p11.setBody(dummyData, "body11"_L1);
    p12.setBody(dummyData, "body12"_L1);

    qfdb.buildMultiPart();
}

QTEST_MAIN(tst_QFormDataBuilder)
#include "tst_qformdatabuilder.moc"
