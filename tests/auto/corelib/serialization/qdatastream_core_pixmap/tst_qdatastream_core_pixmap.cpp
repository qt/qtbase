// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtGui/QPixmap>
#include <QtGui/QImage>

class tst_QDataStreamPixmap : public QObject
{
Q_OBJECT

private slots:
    void stream_with_pixmap();
};

void tst_QDataStreamPixmap::stream_with_pixmap()
{
    // This is a QVariantMap with a 3x3 red QPixmap and two strings inside
    const QByteArray ba = QByteArray::fromBase64(
        "AAAAAwAAAAIAegAAAAoAAAAACgB0AGgAZQByAGUAAAACAHAAAABBAAAAAAGJUE5H"
        "DQoaCgAAAA1JSERSAAAAAwAAAAMIAgAAANlKIugAAAAJcEhZcwAADsQAAA7EAZUr"
        "DhsAAAAQSURBVAiZY/zPAAVMDJgsAB1bAQXZn5ieAAAAAElFTkSuQmCCAAAAAgBh"
        "AAAACgAAAAAKAGgAZQBsAGwAbw==");
    QImage dummy; // Needed to make sure qtGui is loaded

    QTest::ignoreMessage(QtWarningMsg, "QPixmap::fromImageInPlace: "
                         "QPixmap cannot be created without a QGuiApplication");

    QVariantMap map;
    QDataStream d(ba);
    d.setVersion(QDataStream::Qt_5_12);
    d >> map;

    QCOMPARE(map["a"].toString(), QString("hello"));
    // The pixmap is null because this is not a QGuiApplication:
    QCOMPARE(map["p"].value<QPixmap>(), QPixmap());
    QCOMPARE(map["z"].toString(), QString("there"));
}

QTEST_GUILESS_MAIN(tst_QDataStreamPixmap)

#include "tst_qdatastream_core_pixmap.moc"
