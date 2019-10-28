/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtGui/QPixmap>
#include <QtGui/QImage>

class tst_QDataStream : public QObject
{
Q_OBJECT

private slots:
    void stream_with_pixmap();
};

void tst_QDataStream::stream_with_pixmap()
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

QTEST_GUILESS_MAIN(tst_QDataStream)

#include "tst_qdatastream_core_pixmap.moc"
