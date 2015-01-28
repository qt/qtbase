/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
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

#include <QTest>
#include <QFrame>
#include <QStyleOptionFrame>
#include <QPixmap>
#include <QStyle>
#include <QStyleFactory>

class tst_QFrame : public QObject
{
    Q_OBJECT
private slots:
    void testDefaults();
    void testInitStyleOption_data();
    void testInitStyleOption();
    void testPainting_data();
    void testPainting();
};

Q_DECLARE_METATYPE(QFrame::Shape)
Q_DECLARE_METATYPE(QFrame::Shadow)

void tst_QFrame::testDefaults()
{
    QFrame frame;
    QCOMPARE(frame.frameStyle(), int(QFrame::NoFrame | QFrame::Plain));
    frame.setFrameShape(QFrame::Box);
    QCOMPARE(frame.frameStyle(), int(QFrame::Box | QFrame::Plain));
    frame.setFrameStyle(QFrame::Box); // no shadow specified!
    QCOMPARE(frame.frameStyle(), int(QFrame::Box));
}

static void provideFrameData()
{
    QTest::addColumn<QString>("basename");
    QTest::addColumn<int>("lineWidth");
    QTest::addColumn<int>("midLineWidth");
    QTest::addColumn<QFrame::Shape>("shape");
    QTest::addColumn<QFrame::Shadow>("shadow");

    for (int lineWidth = 0; lineWidth < 3; ++lineWidth) {
        for (int midLineWidth = 0; midLineWidth < 3; ++midLineWidth) {
            QTest::newRow(qPrintable(QStringLiteral("box_noshadow_%1_%2").arg(lineWidth).arg(midLineWidth)))
                    << "box_noshadow" << lineWidth << midLineWidth << QFrame::Box << (QFrame::Shadow)0;
            QTest::newRow(qPrintable(QStringLiteral("box_plain_%1_%2").arg(lineWidth).arg(midLineWidth)))
                    << "box_plain" << lineWidth << midLineWidth << QFrame::Box << QFrame::Plain;
            QTest::newRow(qPrintable(QStringLiteral("box_raised_%1_%2").arg(lineWidth).arg(midLineWidth)))
                    << "box_raised" << lineWidth << midLineWidth << QFrame::Box << QFrame::Raised;
            QTest::newRow(qPrintable(QStringLiteral("box_sunken_%1_%2").arg(lineWidth).arg(midLineWidth)))
                    << "box_sunken" << lineWidth << midLineWidth << QFrame::Box << QFrame::Sunken;

            QTest::newRow(qPrintable(QStringLiteral("winpanel_noshadow_%1_%2").arg(lineWidth).arg(midLineWidth)))
                    << "winpanel_noshadow" << lineWidth << midLineWidth << QFrame::WinPanel << (QFrame::Shadow)0;
            QTest::newRow(qPrintable(QStringLiteral("winpanel_plain_%1_%2").arg(lineWidth).arg(midLineWidth)))
                    << "winpanel_plain" << lineWidth << midLineWidth << QFrame::WinPanel << QFrame::Plain;
            QTest::newRow(qPrintable(QStringLiteral("winpanel_raised_%1_%2").arg(lineWidth).arg(midLineWidth)))
                    << "winpanel_raised" << lineWidth << midLineWidth << QFrame::WinPanel << QFrame::Raised;
            QTest::newRow(qPrintable(QStringLiteral("winpanel_sunken_%1_%2").arg(lineWidth).arg(midLineWidth)))
                    << "winpanel_sunken" << lineWidth << midLineWidth << QFrame::WinPanel << QFrame::Sunken;
        }
    }
}

class Frame : public QFrame
{
public:
    using QFrame::initStyleOption;
};

void tst_QFrame::testInitStyleOption_data()
{
    provideFrameData();
}

void tst_QFrame::testInitStyleOption()
{
    QFETCH(QString, basename);
    QFETCH(int, lineWidth);
    QFETCH(int, midLineWidth);
    QFETCH(QFrame::Shape, shape);
    QFETCH(QFrame::Shadow, shadow);

    Frame frame;
    frame.setFrameStyle(shape | shadow);
    frame.setLineWidth(lineWidth);
    frame.setMidLineWidth(midLineWidth);
    frame.resize(16, 16);

    QStyleOptionFrame styleOption;
    frame.initStyleOption(&styleOption);

    switch (shape) {
    case QFrame::Box:
    case QFrame::Panel:
    case QFrame::StyledPanel:
    case QFrame::HLine:
    case QFrame::VLine:
        QCOMPARE(styleOption.lineWidth, lineWidth);
        QCOMPARE(styleOption.midLineWidth, midLineWidth);
        break;

    case QFrame::NoFrame:
    case QFrame::WinPanel:
        QCOMPARE(styleOption.lineWidth, frame.frameWidth());
        QCOMPARE(styleOption.midLineWidth, 0);
        break;
    }

    QCOMPARE(styleOption.features, QStyleOptionFrame::None);
    QCOMPARE(styleOption.frameShape, shape);
    if (shadow == QFrame::Sunken)
        QVERIFY(styleOption.state & QStyle::State_Sunken);
    else if (shadow == QFrame::Raised)
        QVERIFY(styleOption.state & QStyle::State_Raised);
}

QT_BEGIN_NAMESPACE
Q_GUI_EXPORT QPalette qt_fusionPalette();
QT_END_NAMESPACE

void tst_QFrame::testPainting_data()
{
    provideFrameData();
}

void tst_QFrame::testPainting()
{
    QFETCH(QString, basename);
    QFETCH(int, lineWidth);
    QFETCH(int, midLineWidth);
    QFETCH(QFrame::Shape, shape);
    QFETCH(QFrame::Shadow, shadow);

    QFrame frame;
    frame.setStyle(QStyleFactory::create(QStringLiteral("fusion")));
    frame.setPalette(qt_fusionPalette());
    frame.setFrameStyle(shape | shadow);
    frame.setLineWidth(lineWidth);
    frame.setMidLineWidth(midLineWidth);
    frame.resize(16, 16);

    const QPixmap pixmap = frame.grab();

    const QString referenceFilePath = QFINDTESTDATA(QStringLiteral("images/%1_%2_%3.png").arg(basename).arg(lineWidth).arg(midLineWidth));
    const QPixmap referencePixmap(referenceFilePath);
    QVERIFY2(!referencePixmap.isNull(), qPrintable(QStringLiteral("Could not load reference pixmap %1").arg(referenceFilePath)));
    QCOMPARE(pixmap, referencePixmap);
}

QTEST_MAIN(tst_QFrame)

#include "tst_qframe.moc"
