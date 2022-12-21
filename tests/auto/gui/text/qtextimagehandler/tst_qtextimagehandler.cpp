/****************************************************************************
 **
 ** Copyright (C) 2020 The Qt Company Ltd.
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

#include <QTest>

#include <QPainter>
#include <private/qtextimagehandler_p.h>

class tst_QTextImageHandler : public QObject
{
    Q_OBJECT

public:
    tst_QTextImageHandler();

private slots:
    void init();
    void cleanup();
    void cleanupTestCase();
    void loadAtNImages_data();
    void loadAtNImages();
};

tst_QTextImageHandler::tst_QTextImageHandler()
{
}

void tst_QTextImageHandler::init()
{
}

void tst_QTextImageHandler::cleanup()
{
}

void tst_QTextImageHandler::cleanupTestCase()
{
}

void tst_QTextImageHandler::loadAtNImages_data()
{
    QTest::addColumn<QString>("imageFile");

    QTest::addRow("file") << QFINDTESTDATA("data/image.png");
    QTest::addRow("file_url") << QString("file:/") + QFINDTESTDATA("data/image.png");
    QTest::addRow("resource") << ":/data/image.png";
    QTest::addRow("qrc_url") << "qrc:/data/image.png";
}

void tst_QTextImageHandler::loadAtNImages()
{
    QFETCH(QString, imageFile);

    QTextDocument doc;
    QTextCursor c(&doc);
    c.insertHtml("<img src=\"" + imageFile + "\">");
    const auto formats = doc.allFormats();
    const auto it = std::find_if(formats.begin(), formats.end(), [](const auto &format){
        return format.objectType() == QTextFormat::ImageObject;
    });
    QVERIFY(it != formats.end());
    const QTextImageFormat format = (*it).toImageFormat();
    QTextImageHandler handler;

    for (const auto &dpr : {1, 2}) {
        QImage img(20, 20, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::white);
        img.setDevicePixelRatio(dpr);
        QPainter p(&img);
        handler.drawObject(&p, QRect(0, 0, 20, 20), &doc, 0, format);
        p.end();
        QVERIFY(!img.isNull());
        const auto expectedColor = dpr == 1 ? Qt::red : Qt::green;
        QCOMPARE(img.pixelColor(0, 0), expectedColor);
    }
}

QTEST_MAIN(tst_QTextImageHandler)
#include "tst_qtextimagehandler.moc"
