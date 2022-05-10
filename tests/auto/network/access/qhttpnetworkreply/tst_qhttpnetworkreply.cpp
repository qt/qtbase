// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>
#include <QtCore/QBuffer>
#include <QtCore/QByteArray>

#include "private/qhttpnetworkconnection_p.h"

class tst_QHttpNetworkReply: public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void parseHeader_data();
    void parseHeader();

    void parseHeaderVerification_data();
    void parseHeaderVerification();

    void parseEndOfHeader_data();
    void parseEndOfHeader();
};

void tst_QHttpNetworkReply::parseHeader_data()
{
    QTest::addColumn<QByteArray>("headers");
    QTest::addColumn<QStringList>("fields");
    QTest::addColumn<QStringList>("values");

    QTest::newRow("no-fields") << QByteArray("\r\n") << QStringList() << QStringList();
    QTest::newRow("empty-field") << QByteArray("Set-Cookie: \r\n")
                                 << (QStringList() << "Set-Cookie")
                                 << (QStringList() << "");
    QTest::newRow("single-field") << QByteArray("Content-Type: text/html; charset=utf-8\r\n")
                                  << (QStringList() << "Content-Type")
                                  << (QStringList() << "text/html; charset=utf-8");
    QTest::newRow("single-field-continued") << QByteArray("Content-Type: text/html;\r\n"
                                                          " charset=utf-8\r\n")
                                            << (QStringList() << "Content-Type")
                                            << (QStringList() << "text/html; charset=utf-8");
    QTest::newRow("single-field-on-five-lines")
            << QByteArray("Name:\r\n first\r\n \r\n \r\n last\r\n") << (QStringList() << "Name")
            << (QStringList() << "first last");

    QTest::newRow("multi-field") << QByteArray("Content-Type: text/html; charset=utf-8\r\n"
                                               "Content-Length: 1024\r\n"
                                               "Content-Encoding: gzip\r\n")
                                 << (QStringList() << "Content-Type" << "Content-Length" << "Content-Encoding")
                                 << (QStringList() << "text/html; charset=utf-8" << "1024" << "gzip");
    QTest::newRow("multi-field-with-emtpy") << QByteArray("Content-Type: text/html; charset=utf-8\r\n"
                                                          "Content-Length: 1024\r\n"
                                                          "Set-Cookie: \r\n"
                                                          "Content-Encoding: gzip\r\n")
                                            << (QStringList() << "Content-Type" << "Content-Length" << "Set-Cookie" << "Content-Encoding")
                                            << (QStringList() << "text/html; charset=utf-8" << "1024" << "" << "gzip");

    QTest::newRow("lws-field") << QByteArray("Content-Type: text/html; charset=utf-8\r\n"
                                             "Content-Length:\r\n 1024\r\n"
                                             "Content-Encoding: gzip\r\n")
                               << (QStringList() << "Content-Type" << "Content-Length" << "Content-Encoding")
                               << (QStringList() << "text/html; charset=utf-8" << "1024" << "gzip");

    QTest::newRow("duplicated-field") << QByteArray("Vary: Accept-Language\r\n"
                                                    "Vary: Cookie\r\n"
                                                    "Vary: User-Agent\r\n")
                                      << (QStringList() << "Vary")
                                      << (QStringList() << "Accept-Language, Cookie, User-Agent");
}

void tst_QHttpNetworkReply::parseHeader()
{
    QFETCH(QByteArray, headers);
    QFETCH(QStringList, fields);
    QFETCH(QStringList, values);

    QHttpNetworkReply reply;
    reply.parseHeader(headers);
    for (int i = 0; i < fields.count(); ++i) {
        //qDebug() << "field" << fields.at(i) << "value" << reply.headerField(fields.at(i)) << "expected" << values.at(i);
        QString field = reply.headerField(fields.at(i).toLatin1());
        QCOMPARE(field, values.at(i));
    }
}

// both constants are taken from the default settings of Apache
// see: http://httpd.apache.org/docs/2.2/mod/core.html#limitrequestfieldsize and
// http://httpd.apache.org/docs/2.2/mod/core.html#limitrequestfields
const int MAX_HEADER_FIELD_SIZE = 8 * 1024;
const int MAX_HEADER_FIELDS = 100;

void tst_QHttpNetworkReply::parseHeaderVerification_data()
{
    QTest::addColumn<QByteArray>("headers");
    QTest::addColumn<bool>("success");

    QTest::newRow("no-header-fields") << QByteArray("\r\n") << true;
    QTest::newRow("starting-with-space") << QByteArray(" Content-Encoding: gzip\r\n") << false;
    QTest::newRow("starting-with-tab") << QByteArray("\tContent-Encoding: gzip\r\n") << false;
    QTest::newRow("only-colon") << QByteArray(":\r\n") << false;
    QTest::newRow("colon-and-value") << QByteArray(": only-value\r\n") << false;
    QTest::newRow("name-with-space") << QByteArray("Content Length: 10\r\n") << false;
    QTest::newRow("missing-colon-1") << QByteArray("Content-Encoding\r\n") << false;
    QTest::newRow("missing-colon-2")
            << QByteArray("Content-Encoding\r\nContent-Length: 10\r\n") << false;
    QTest::newRow("missing-colon-3")
            << QByteArray("Content-Encoding: gzip\r\nContent-Length\r\n") << false;
    QTest::newRow("header-field-too-long")
            << (QByteArray("Content-Type: ") + QByteArray(MAX_HEADER_FIELD_SIZE, 'a')
                + QByteArray("\r\n"))
            << false;

    QByteArray name = "Content-Type: ";
    QTest::newRow("max-header-field-size")
            << (name + QByteArray(MAX_HEADER_FIELD_SIZE - name.size(), 'a') + QByteArray("\r\n"))
            << true;

    QByteArray tooManyHeaders = QByteArray("Content-Type: text/html; charset=utf-8\r\n")
                                        .repeated(MAX_HEADER_FIELDS + 1);
    QTest::newRow("too-many-headers") << tooManyHeaders << false;

    QByteArray maxHeaders =
            QByteArray("Content-Type: text/html; charset=utf-8\r\n").repeated(MAX_HEADER_FIELDS);
    QTest::newRow("max-headers") << maxHeaders << true;

    QByteArray firstValue(MAX_HEADER_FIELD_SIZE / 2, 'a');
    constexpr int obsFold = 1;
    QTest::newRow("max-continuation-size")
            << (name + firstValue + QByteArray("\r\n ")
                + QByteArray(MAX_HEADER_FIELD_SIZE - name.size() - firstValue.size() - obsFold, 'b')
                + QByteArray("\r\n"))
            << true;
    QTest::newRow("too-long-continuation-size")
            << (name + firstValue + QByteArray("\r\n ")
                + QByteArray(MAX_HEADER_FIELD_SIZE - name.size() - firstValue.size() - obsFold + 1,
                             'b')
                + QByteArray("\r\n"))
            << false;
}

void tst_QHttpNetworkReply::parseHeaderVerification()
{
    QFETCH(QByteArray, headers);
    QFETCH(bool, success);
    QHttpNetworkReply reply;
    reply.parseHeader(headers);
    if (success && QByteArrayView(headers).trimmed().size())
        QVERIFY(reply.header().size() > 0);
    else
        QCOMPARE(reply.header().size(), 0);
}

class TestHeaderSocket : public QAbstractSocket
{
public:
    explicit TestHeaderSocket(const QByteArray &input) : QAbstractSocket(QAbstractSocket::TcpSocket, nullptr)
    {
        inputBuffer.setData(input);
        inputBuffer.open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    }

    qint64 readData(char *data, qint64 maxlen) override { return inputBuffer.read(data, maxlen); }

    QBuffer inputBuffer;
};

class TestHeaderReply : public QHttpNetworkReply
{
public:
    QHttpNetworkReplyPrivate *replyPrivate() { return static_cast<QHttpNetworkReplyPrivate *>(d_ptr.data()); }
};

void tst_QHttpNetworkReply::parseEndOfHeader_data()
{
    QTest::addColumn<QByteArray>("headers");
    QTest::addColumn<qint64>("lengths");

    QTest::newRow("CRLFCRLF") << QByteArray("Content-Type: text/html; charset=utf-8\r\n"
                                            "Content-Length:\r\n 1024\r\n"
                                            "Content-Encoding: gzip\r\n\r\nHTTPBODY")
                               << qint64(90);

    QTest::newRow("CRLFLF") << QByteArray("Content-Type: text/html; charset=utf-8\r\n"
                                          "Content-Length:\r\n 1024\r\n"
                                          "Content-Encoding: gzip\r\n\nHTTPBODY")
                            << qint64(89);

    QTest::newRow("LFCRLF") << QByteArray("Content-Type: text/html; charset=utf-8\r\n"
                                          "Content-Length:\r\n 1024\r\n"
                                          "Content-Encoding: gzip\n\r\nHTTPBODY")
                            << qint64(89);

    QTest::newRow("LFLF") << QByteArray("Content-Type: text/html; charset=utf-8\r\n"
                                        "Content-Length:\r\n 1024\r\n"
                                        "Content-Encoding: gzip\n\nHTTPBODY")
                          << qint64(88);
}

void tst_QHttpNetworkReply::parseEndOfHeader()
{
    QFETCH(QByteArray, headers);
    QFETCH(qint64, lengths);

    TestHeaderSocket socket(headers);

    TestHeaderReply reply;

    QHttpNetworkReplyPrivate *replyPrivate = reply.replyPrivate();
    qint64 headerBytes = replyPrivate->readHeader(&socket);
    QCOMPARE(headerBytes, lengths);
}

QTEST_MAIN(tst_QHttpNetworkReply)
#include "tst_qhttpnetworkreply.moc"
