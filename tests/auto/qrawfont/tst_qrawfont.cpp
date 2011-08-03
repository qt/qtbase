/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include <qrawfont.h>
#include <private/qrawfont_p.h>

class tst_QRawFont: public QObject
{
    Q_OBJECT
#if !defined(QT_NO_RAWFONT)
private slots:
    void invalidRawFont();

    void explicitRawFontNotLoadedInDatabase_data();
    void explicitRawFontNotLoadedInDatabase();

    void explicitRawFontNotAvailableInSystem_data();
    void explicitRawFontNotAvailableInSystem();

    void correctFontData_data();
    void correctFontData();

    void glyphIndices();

    void advances_data();
    void advances();

    void textLayout();

    void fontTable_data();
    void fontTable();

    void supportedWritingSystems_data();
    void supportedWritingSystems();

    void supportsCharacter_data();
    void supportsCharacter();

    void supportsUcs4Character_data();
    void supportsUcs4Character();

    void fromFont_data();
    void fromFont();

    void copyConstructor_data();
    void copyConstructor();

    void detach_data();
    void detach();

    void unsupportedWritingSystem_data();
    void unsupportedWritingSystem();

    void rawFontSetPixelSize_data();
    void rawFontSetPixelSize();

#if defined(Q_WS_X11) || defined(Q_WS_MAC) && defined(QT_MAC_USE_COCOA)
    void multipleRawFontsFromData();
#endif

#endif // QT_NO_RAWFONT
};

#if !defined(QT_NO_RAWFONT)
Q_DECLARE_METATYPE(QFont::HintingPreference)
Q_DECLARE_METATYPE(QFont::Style)
Q_DECLARE_METATYPE(QFont::Weight)
Q_DECLARE_METATYPE(QFontDatabase::WritingSystem)

void tst_QRawFont::invalidRawFont()
{
    QRawFont font;
    QVERIFY(!font.isValid());
    QCOMPARE(font.pixelSize(), 0.0);
    QVERIFY(font.familyName().isEmpty());
    QCOMPARE(font.style(), QFont::StyleNormal);
    QCOMPARE(font.weight(), -1);
    QCOMPARE(font.ascent(), 0.0);
    QCOMPARE(font.descent(), 0.0);
    QVERIFY(font.glyphIndexesForString(QLatin1String("Test")).isEmpty());
}

void tst_QRawFont::explicitRawFontNotLoadedInDatabase_data()
{
    QTest::addColumn<QFont::HintingPreference>("hintingPreference");

    QTest::newRow("Default hinting preference") << QFont::PreferDefaultHinting;
    QTest::newRow("No hinting") << QFont::PreferNoHinting;
    QTest::newRow("Vertical hinting") << QFont::PreferVerticalHinting;
    QTest::newRow("Full hinting") << QFont::PreferFullHinting;
}

void tst_QRawFont::explicitRawFontNotLoadedInDatabase()
{
    QFETCH(QFont::HintingPreference, hintingPreference);

    QRawFont font(QLatin1String(SRCDIR "testfont.ttf"), 10, hintingPreference);
    QVERIFY(font.isValid());

    QVERIFY(!QFontDatabase().families().contains(font.familyName()));
}

void tst_QRawFont::explicitRawFontNotAvailableInSystem_data()
{
    QTest::addColumn<QFont::HintingPreference>("hintingPreference");

    QTest::newRow("Default hinting preference") << QFont::PreferDefaultHinting;
    QTest::newRow("No hinting") << QFont::PreferNoHinting;
    QTest::newRow("Vertical hinting") << QFont::PreferVerticalHinting;
    QTest::newRow("Full hinting") << QFont::PreferFullHinting;
}

void tst_QRawFont::explicitRawFontNotAvailableInSystem()
{
    QFETCH(QFont::HintingPreference, hintingPreference);

    QRawFont rawfont(QLatin1String(SRCDIR "testfont.ttf"), 10, hintingPreference);

    {
        QFont font(rawfont.familyName(), 10);

        QVERIFY(!font.exactMatch());
        QVERIFY(font.family() != QFontInfo(font).family());
    }
}

void tst_QRawFont::correctFontData_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("expectedFamilyName");
    QTest::addColumn<QFont::Style>("style");
    QTest::addColumn<QFont::Weight>("weight");
    QTest::addColumn<QFont::HintingPreference>("hintingPreference");
    QTest::addColumn<qreal>("unitsPerEm");
    QTest::addColumn<qreal>("pixelSize");

    int hintingPreferences[] = {
        int(QFont::PreferDefaultHinting),
        int(QFont::PreferNoHinting),
        int(QFont::PreferVerticalHinting),
        int(QFont::PreferFullHinting),
        -1
    };
    int *hintingPreference = hintingPreferences;

    while (*hintingPreference >= 0) {
        QString fileName = QLatin1String(SRCDIR "testfont.ttf");
        QString title = fileName
                      + QLatin1String(": hintingPreference=")
                      + QString::number(*hintingPreference);

        QTest::newRow(qPrintable(title))
                << fileName
                << QString::fromLatin1("QtBidiTestFont")
                << QFont::StyleNormal
                << QFont::Normal
                << QFont::HintingPreference(*hintingPreference)
                << 1000.0
                << 10.0;

        fileName = QLatin1String(SRCDIR "testfont_bold_italic.ttf");
        title = fileName
              + QLatin1String(": hintingPreference=")
              + QString::number(*hintingPreference);

        QTest::newRow(qPrintable(title))
                << fileName
                << QString::fromLatin1("QtBidiTestFont")
                << QFont::StyleItalic
                << QFont::Bold
                << QFont::HintingPreference(*hintingPreference)
                << 1000.0
                << 10.0;

        ++hintingPreference;
    }
}

void tst_QRawFont::correctFontData()
{
    QFETCH(QString, fileName);
    QFETCH(QString, expectedFamilyName);
    QFETCH(QFont::Style, style);
    QFETCH(QFont::Weight, weight);
    QFETCH(QFont::HintingPreference, hintingPreference);
    QFETCH(qreal, unitsPerEm);
    QFETCH(qreal, pixelSize);

    QRawFont font(fileName, 10, hintingPreference);
    QVERIFY(font.isValid());

    QCOMPARE(font.familyName(), expectedFamilyName);
    QCOMPARE(font.style(), style);
    QCOMPARE(font.weight(), int(weight));
    QCOMPARE(font.hintingPreference(), hintingPreference);
    QCOMPARE(font.unitsPerEm(), unitsPerEm);
    QCOMPARE(font.pixelSize(), pixelSize);
}

void tst_QRawFont::glyphIndices()
{
    QRawFont font(QLatin1String(SRCDIR "testfont.ttf"), 10);
    QVERIFY(font.isValid());

    QVector<quint32> glyphIndices = font.glyphIndexesForString(QLatin1String("Foobar"));
    QVector<quint32> expectedGlyphIndices;
    expectedGlyphIndices << 44 << 83 << 83 << 70 << 69 << 86;

    QCOMPARE(glyphIndices, expectedGlyphIndices);
}

void tst_QRawFont::advances_data()
{
    QTest::addColumn<QFont::HintingPreference>("hintingPreference");

    QTest::newRow("Default hinting preference") << QFont::PreferDefaultHinting;
    QTest::newRow("No hinting") << QFont::PreferNoHinting;
    QTest::newRow("Vertical hinting") << QFont::PreferVerticalHinting;
    QTest::newRow("Full hinting") << QFont::PreferFullHinting;
}

void tst_QRawFont::advances()
{
    QFETCH(QFont::HintingPreference, hintingPreference);

    QRawFont font(QLatin1String(SRCDIR "testfont.ttf"), 10, hintingPreference);
    QVERIFY(font.isValid());

    QRawFontPrivate *font_d = QRawFontPrivate::get(font);
    QVERIFY(font_d->fontEngine != 0);

    QVector<quint32> glyphIndices;
    glyphIndices << 44 << 83 << 83 << 70 << 69 << 86; // "Foobar"

    bool supportsSubPixelPositions = font_d->fontEngine->supportsSubPixelPositions();
    QVector<QPointF> advances = font.advancesForGlyphIndexes(glyphIndices);
    for (int i=0; i<glyphIndices.size(); ++i) {
        QVERIFY(qFuzzyCompare(qRound(advances.at(i).x()), 8.0));
        if (supportsSubPixelPositions)
            QVERIFY(advances.at(i).x() > 8.0);

        QVERIFY(qFuzzyIsNull(advances.at(i).y()));
    }
}

void tst_QRawFont::textLayout()
{
    QFontDatabase fontDatabase;
    int id = fontDatabase.addApplicationFont(SRCDIR "testfont.ttf");
    QVERIFY(id >= 0);

    QString familyName = QString::fromLatin1("QtBidiTestFont");
    QFont font(familyName);
    font.setPixelSize(18.0);
    QCOMPARE(QFontInfo(font).family(), familyName);

    QTextLayout layout(QLatin1String("Foobar"));
    layout.setFont(font);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    QList<QGlyphRun> glyphRuns = layout.glyphRuns();
    QCOMPARE(glyphRuns.size(), 1);

    QGlyphRun glyphs = glyphRuns.at(0);

    QRawFont rawFont = glyphs.rawFont();
    QVERIFY(rawFont.isValid());
    QCOMPARE(rawFont.familyName(), familyName);
    QCOMPARE(rawFont.pixelSize(), 18.0);

    QVector<quint32> expectedGlyphIndices;
    expectedGlyphIndices << 44 << 83 << 83 << 70 << 69 << 86;

    QCOMPARE(glyphs.glyphIndexes(), expectedGlyphIndices);

    QVERIFY(fontDatabase.removeApplicationFont(id));
}

void tst_QRawFont::fontTable_data()
{
    QTest::addColumn<QByteArray>("tagName");
    QTest::addColumn<QFont::HintingPreference>("hintingPreference");
    QTest::addColumn<int>("offset");
    QTest::addColumn<quint32>("expectedValue");

    QTest::newRow("Head table, magic number, default hinting")
            << QByteArray("head")
            << QFont::PreferDefaultHinting
            << 12
            << (QSysInfo::ByteOrder == QSysInfo::BigEndian
                ? 0x5F0F3CF5
                : 0xF53C0F5F);

    QTest::newRow("Head table, magic number, no hinting")
            << QByteArray("head")
            << QFont::PreferNoHinting
            << 12
            << (QSysInfo::ByteOrder == QSysInfo::BigEndian
                ? 0x5F0F3CF5
                : 0xF53C0F5F);

    QTest::newRow("Head table, magic number, vertical hinting")
            << QByteArray("head")
            << QFont::PreferVerticalHinting
            << 12
            << (QSysInfo::ByteOrder == QSysInfo::BigEndian
                ? 0x5F0F3CF5
                : 0xF53C0F5F);

    QTest::newRow("Head table, magic number, full hinting")
            << QByteArray("head")
            << QFont::PreferFullHinting
            << 12
            << (QSysInfo::ByteOrder == QSysInfo::BigEndian
                ? 0x5F0F3CF5
                : 0xF53C0F5F);
}

void tst_QRawFont::fontTable()
{
    QFETCH(QByteArray, tagName);
    QFETCH(QFont::HintingPreference, hintingPreference);
    QFETCH(int, offset);
    QFETCH(quint32, expectedValue);

    QRawFont font(QString::fromLatin1(SRCDIR "testfont.ttf"), 10, hintingPreference);
    QVERIFY(font.isValid());

    QByteArray table = font.fontTable(tagName);
    QVERIFY(!table.isEmpty());

    const quint32 *value = reinterpret_cast<const quint32 *>(table.constData() + offset);
    QCOMPARE(*value, expectedValue);
}

typedef QList<QFontDatabase::WritingSystem> WritingSystemList;
Q_DECLARE_METATYPE(WritingSystemList)

void tst_QRawFont::supportedWritingSystems_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<WritingSystemList>("writingSystems");
    QTest::addColumn<QFont::HintingPreference>("hintingPreference");

    for (int hintingPreference=QFont::PreferDefaultHinting;
         hintingPreference<=QFont::PreferFullHinting;
         ++hintingPreference) {

        QTest::newRow(qPrintable(QString::fromLatin1("testfont.ttf, hintingPreference=%1")
                      .arg(hintingPreference)))
            << QString::fromLatin1(SRCDIR "testfont.ttf")
            << (QList<QFontDatabase::WritingSystem>()
                  << QFontDatabase::Latin
                  << QFontDatabase::Hebrew
                  << QFontDatabase::Vietnamese) // Vietnamese uses same unicode bits as Latin
            << QFont::HintingPreference(hintingPreference);

        QTest::newRow(qPrintable(QString::fromLatin1("testfont_bold_italic.ttf, hintingPreference=%1")
                      .arg(hintingPreference)))
            << QString::fromLatin1(SRCDIR "testfont_bold_italic.ttf")
            << (QList<QFontDatabase::WritingSystem>()
                    << QFontDatabase::Latin
                    << QFontDatabase::Hebrew
                    << QFontDatabase::Devanagari
                    << QFontDatabase::Vietnamese) // Vietnamese uses same unicode bits as Latin
            << QFont::HintingPreference(hintingPreference);
    }
}

void tst_QRawFont::supportedWritingSystems()
{
    QFETCH(QString, fileName);
    QFETCH(WritingSystemList, writingSystems);
    QFETCH(QFont::HintingPreference, hintingPreference);

    QRawFont font(fileName, 10, hintingPreference);
    QVERIFY(font.isValid());

    WritingSystemList actualWritingSystems = font.supportedWritingSystems();
    QCOMPARE(actualWritingSystems.size(), writingSystems.size());

    foreach (QFontDatabase::WritingSystem writingSystem, writingSystems)
        QVERIFY(actualWritingSystems.contains(writingSystem));
}

void tst_QRawFont::supportsCharacter_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QFont::HintingPreference>("hintingPreference");
    QTest::addColumn<QChar>("character");
    QTest::addColumn<bool>("shouldBeSupported");

    const char *fileNames[2] = {
        SRCDIR "testfont.ttf",
        SRCDIR "testfont_bold_italic.ttf"
    };

    for (int hintingPreference=QFont::PreferDefaultHinting;
         hintingPreference<=QFont::PreferFullHinting;
         ++hintingPreference) {

        for (int i=0; i<2; ++i) {
            QString fileName = QLatin1String(fileNames[i]);

            // Latin text
            for (char ch='!'; ch<='~'; ++ch) {
                    QString title = QString::fromLatin1("%1, character=0x%2, hintingPreference=%3")
                            .arg(fileName).arg(QString::number(ch, 16)).arg(hintingPreference);

                    QTest::newRow(qPrintable(title))
                            << fileName
                            << QFont::HintingPreference(hintingPreference)
                            << QChar::fromLatin1(ch)
                            << true;
            }

            // Hebrew text
            for (quint16 ch=0x05D0; ch<=0x05EA; ++ch) {
                QString title = QString::fromLatin1("%1, character=0x%2, hintingPreference=%3")
                        .arg(fileName).arg(QString::number(ch, 16)).arg(hintingPreference);

                QTest::newRow(qPrintable(title))
                        << fileName
                        << QFont::HintingPreference(hintingPreference)
                        << QChar(ch)
                        << true;
            }

            QTest::newRow(qPrintable(QString::fromLatin1("Missing character, %1, hintingPreference=%2")
                          .arg(fileName).arg(hintingPreference)))
                    << fileName
                    << QFont::HintingPreference(hintingPreference)
                    << QChar(0xD8)
                    << false;
        }
    }
}

void tst_QRawFont::supportsCharacter()
{
    QFETCH(QString, fileName);
    QFETCH(QFont::HintingPreference, hintingPreference);
    QFETCH(QChar, character);
    QFETCH(bool, shouldBeSupported);

    QRawFont font(fileName, 10, hintingPreference);
    QVERIFY(font.isValid());

    QCOMPARE(font.supportsCharacter(character), shouldBeSupported);
}

void tst_QRawFont::supportsUcs4Character_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QFont::HintingPreference>("hintingPreference");
    QTest::addColumn<quint32>("ucs4");
    QTest::addColumn<bool>("shouldBeSupported");

    // Gothic text
    for (int hintingPreference=QFont::PreferDefaultHinting;
         hintingPreference<=QFont::PreferFullHinting;
         ++hintingPreference) {
        for (quint32 ch=0x10330; ch<=0x1034A; ++ch) {
            {
                QString fileName = QString::fromLatin1(SRCDIR "testfont.ttf");
                QString title = QString::fromLatin1("%1, character=0x%2, hintingPreference=%3")
                        .arg(fileName).arg(QString::number(ch, 16)).arg(hintingPreference);

                QTest::newRow(qPrintable(title))
                        << fileName
                        << QFont::HintingPreference(hintingPreference)
                        << ch
                        << true;
            }

            {
                QString fileName = QString::fromLatin1(SRCDIR "testfont_bold_italic.ttf");
                QString title = QString::fromLatin1("%1, character=0x%2, hintingPreference=%3")
                        .arg(fileName).arg(QString::number(ch, 16)).arg(hintingPreference);

                QTest::newRow(qPrintable(title))
                        << fileName
                        << QFont::HintingPreference(hintingPreference)
                        << ch
                        << false;
            }
        }
    }
}

void tst_QRawFont::supportsUcs4Character()
{
    QFETCH(QString, fileName);
    QFETCH(QFont::HintingPreference, hintingPreference);
    QFETCH(quint32, ucs4);
    QFETCH(bool, shouldBeSupported);

    QRawFont font(fileName, 10, hintingPreference);
    QVERIFY(font.isValid());

    QCOMPARE(font.supportsCharacter(ucs4), shouldBeSupported);
}

void tst_QRawFont::fromFont_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QFont::HintingPreference>("hintingPreference");
    QTest::addColumn<QString>("familyName");
    QTest::addColumn<QFontDatabase::WritingSystem>("writingSystem");

    for (int i=QFont::PreferDefaultHinting; i<=QFont::PreferFullHinting; ++i) {
        QString titleBase = QString::fromLatin1("%2, hintingPreference=%1, writingSystem=%3")
                .arg(i);
        {
            QString fileName = QString::fromLatin1(SRCDIR "testfont.ttf");
            QFontDatabase::WritingSystem writingSystem = QFontDatabase::Any;

            QString title = titleBase.arg(fileName).arg(writingSystem);
            QTest::newRow(qPrintable(title))
                    << fileName
                    << QFont::HintingPreference(i)
                    << "QtBidiTestFont"
                    << writingSystem;
        }

        {
            QString fileName = QString::fromLatin1(SRCDIR "testfont.ttf");
            QFontDatabase::WritingSystem writingSystem = QFontDatabase::Hebrew;

            QString title = titleBase.arg(fileName).arg(writingSystem);
            QTest::newRow(qPrintable(title))
                    << fileName
                    << QFont::HintingPreference(i)
                    << "QtBidiTestFont"
                    << writingSystem;
        }

        {
            QString fileName = QString::fromLatin1(SRCDIR "testfont.ttf");
            QFontDatabase::WritingSystem writingSystem = QFontDatabase::Latin;

            QString title = titleBase.arg(fileName).arg(writingSystem);
            QTest::newRow(qPrintable(title))
                    << fileName
                    << QFont::HintingPreference(i)
                    << "QtBidiTestFont"
                    << writingSystem;
        }
    }
}

void tst_QRawFont::fromFont()
{
    QFETCH(QString, fileName);
    QFETCH(QFont::HintingPreference, hintingPreference);
    QFETCH(QString, familyName);
    QFETCH(QFontDatabase::WritingSystem, writingSystem);

    QFontDatabase fontDatabase;
    int id = fontDatabase.addApplicationFont(fileName);
    QVERIFY(id >= 0);

    QFont font(familyName);
    font.setHintingPreference(hintingPreference);
    font.setPixelSize(26.0);

    QRawFont rawFont = QRawFont::fromFont(font, writingSystem);
    QVERIFY(rawFont.isValid());
    QCOMPARE(rawFont.familyName(), familyName);
    QCOMPARE(rawFont.pixelSize(), 26.0);

    QVERIFY(fontDatabase.removeApplicationFont(id));
}

void tst_QRawFont::copyConstructor_data()
{
    QTest::addColumn<QFont::HintingPreference>("hintingPreference");

    QTest::newRow("Default hinting preference") << QFont::PreferDefaultHinting;
    QTest::newRow("No hinting preference") << QFont::PreferNoHinting;
    QTest::newRow("Vertical hinting preference") << QFont::PreferVerticalHinting;
    QTest::newRow("Full hinting preference") << QFont::PreferFullHinting;
}

void tst_QRawFont::copyConstructor()
{
    QFETCH(QFont::HintingPreference, hintingPreference);

    {
        QString rawFontFamilyName;
        qreal rawFontPixelSize;
        qreal rawFontAscent;
        qreal rawFontDescent;
        int rawFontTableSize;

        QRawFont outerRawFont;
        {
            QRawFont rawFont(QString::fromLatin1(SRCDIR "testfont.ttf"), 11, hintingPreference);
            QVERIFY(rawFont.isValid());

            rawFontFamilyName = rawFont.familyName();
            rawFontPixelSize = rawFont.pixelSize();
            rawFontAscent = rawFont.ascent();
            rawFontDescent = rawFont.descent();
            rawFontTableSize = rawFont.fontTable("glyf").size();
            QVERIFY(rawFontTableSize > 0);

            {
                QRawFont otherRawFont(rawFont);
                QVERIFY(otherRawFont.isValid());
                QCOMPARE(otherRawFont.pixelSize(), rawFontPixelSize);
                QCOMPARE(otherRawFont.familyName(), rawFontFamilyName);
                QCOMPARE(otherRawFont.hintingPreference(), hintingPreference);
                QCOMPARE(otherRawFont.ascent(), rawFontAscent);
                QCOMPARE(otherRawFont.descent(), rawFontDescent);
                QCOMPARE(otherRawFont.fontTable("glyf").size(), rawFontTableSize);
            }

            {
                QRawFont otherRawFont = rawFont;
                QVERIFY(otherRawFont.isValid());
                QCOMPARE(otherRawFont.pixelSize(), rawFontPixelSize);
                QCOMPARE(otherRawFont.familyName(), rawFontFamilyName);
                QCOMPARE(otherRawFont.hintingPreference(), hintingPreference);
                QCOMPARE(otherRawFont.ascent(), rawFontAscent);
                QCOMPARE(otherRawFont.descent(), rawFontDescent);
                QCOMPARE(otherRawFont.fontTable("glyf").size(), rawFontTableSize);
            }

            outerRawFont = rawFont;
        }

        QVERIFY(outerRawFont.isValid());
        QCOMPARE(outerRawFont.pixelSize(), rawFontPixelSize);
        QCOMPARE(outerRawFont.familyName(), rawFontFamilyName);
        QCOMPARE(outerRawFont.hintingPreference(), hintingPreference);
        QCOMPARE(outerRawFont.ascent(), rawFontAscent);
        QCOMPARE(outerRawFont.descent(), rawFontDescent);
        QCOMPARE(outerRawFont.fontTable("glyf").size(), rawFontTableSize);
    }
}

void tst_QRawFont::detach_data()
{
    QTest::addColumn<QFont::HintingPreference>("hintingPreference");

    QTest::newRow("Default hinting preference") << QFont::PreferDefaultHinting;
    QTest::newRow("No hinting preference") << QFont::PreferNoHinting;
    QTest::newRow("Vertical hinting preference") << QFont::PreferVerticalHinting;
    QTest::newRow("Full hinting preference") << QFont::PreferFullHinting;
}

void tst_QRawFont::detach()
{
    QFETCH(QFont::HintingPreference, hintingPreference);

    {
        QString rawFontFamilyName;
        qreal rawFontPixelSize;
        qreal rawFontAscent;
        qreal rawFontDescent;
        int rawFontTableSize;

        QRawFont outerRawFont;
        {
            QRawFont rawFont(QString::fromLatin1(SRCDIR "testfont.ttf"), 11, hintingPreference);
            QVERIFY(rawFont.isValid());

            rawFontFamilyName = rawFont.familyName();
            rawFontPixelSize = rawFont.pixelSize();
            rawFontAscent = rawFont.ascent();
            rawFontDescent = rawFont.descent();
            rawFontTableSize = rawFont.fontTable("glyf").size();
            QVERIFY(rawFontTableSize > 0);

            {
                QRawFont otherRawFont(rawFont);

                otherRawFont.loadFromFile(QLatin1String(SRCDIR "testfont.ttf"),
                                          rawFontPixelSize, hintingPreference);

                QVERIFY(otherRawFont.isValid());
                QCOMPARE(otherRawFont.pixelSize(), rawFontPixelSize);
                QCOMPARE(otherRawFont.familyName(), rawFontFamilyName);
                QCOMPARE(otherRawFont.hintingPreference(), hintingPreference);
                QCOMPARE(otherRawFont.ascent(), rawFontAscent);
                QCOMPARE(otherRawFont.descent(), rawFontDescent);
                QCOMPARE(otherRawFont.fontTable("glyf").size(), rawFontTableSize);
            }

            {
                QRawFont otherRawFont = rawFont;

                otherRawFont.loadFromFile(QLatin1String(SRCDIR "testfont.ttf"),
                                          rawFontPixelSize, hintingPreference);

                QVERIFY(otherRawFont.isValid());
                QCOMPARE(otherRawFont.pixelSize(), rawFontPixelSize);
                QCOMPARE(otherRawFont.familyName(), rawFontFamilyName);
                QCOMPARE(otherRawFont.hintingPreference(), hintingPreference);
                QCOMPARE(otherRawFont.ascent(), rawFontAscent);
                QCOMPARE(otherRawFont.descent(), rawFontDescent);
                QCOMPARE(otherRawFont.fontTable("glyf").size(), rawFontTableSize);
            }

            outerRawFont = rawFont;

            rawFont.loadFromFile(QLatin1String(SRCDIR "testfont.ttf"), rawFontPixelSize,
                                 hintingPreference);
        }

        QVERIFY(outerRawFont.isValid());
        QCOMPARE(outerRawFont.pixelSize(), rawFontPixelSize);
        QCOMPARE(outerRawFont.familyName(), rawFontFamilyName);
        QCOMPARE(outerRawFont.hintingPreference(), hintingPreference);
        QCOMPARE(outerRawFont.ascent(), rawFontAscent);
        QCOMPARE(outerRawFont.descent(), rawFontDescent);
        QCOMPARE(outerRawFont.fontTable("glyf").size(), rawFontTableSize);
    }
}

void tst_QRawFont::unsupportedWritingSystem_data()
{
    QTest::addColumn<QFont::HintingPreference>("hintingPreference");

    QTest::newRow("Default hinting preference") << QFont::PreferDefaultHinting;
    QTest::newRow("No hinting preference") << QFont::PreferNoHinting;
    QTest::newRow("Vertical hinting preference") << QFont::PreferVerticalHinting;
    QTest::newRow("Full hinting preference") << QFont::PreferFullHinting;
}

void tst_QRawFont::unsupportedWritingSystem()
{
    QFETCH(QFont::HintingPreference, hintingPreference);

    QFontDatabase fontDatabase;
    int id = fontDatabase.addApplicationFont(QLatin1String(SRCDIR "testfont.ttf"));

    QFont font("QtBidiTestFont");
    font.setHintingPreference(hintingPreference);
    font.setPixelSize(12.0);

    QRawFont rawFont = QRawFont::fromFont(font, QFontDatabase::Any);
    QCOMPARE(rawFont.familyName(), QString::fromLatin1("QtBidiTestFont"));
    QCOMPARE(rawFont.pixelSize(), 12.0);

    rawFont = QRawFont::fromFont(font, QFontDatabase::Hebrew);
    QCOMPARE(rawFont.familyName(), QString::fromLatin1("QtBidiTestFont"));
    QCOMPARE(rawFont.pixelSize(), 12.0);

    QString arabicText = QFontDatabase::writingSystemSample(QFontDatabase::Arabic);

    QTextLayout layout;
    layout.setFont(font);
    layout.setText(arabicText);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    QList<QGlyphRun> glyphRuns = layout.glyphRuns();
    QCOMPARE(glyphRuns.size(), 1);

    QGlyphRun glyphs = glyphRuns.at(0);
    QRawFont layoutFont = glyphs.rawFont();
    QVERIFY(layoutFont.familyName() != QString::fromLatin1("QtBidiTestFont"));
    QCOMPARE(layoutFont.pixelSize(), 12.0);

    rawFont = QRawFont::fromFont(font, QFontDatabase::Arabic);
    QCOMPARE(rawFont.familyName(), layoutFont.familyName());
    QCOMPARE(rawFont.pixelSize(), 12.0);

    fontDatabase.removeApplicationFont(id);
}

void tst_QRawFont::rawFontSetPixelSize_data()
{
    QTest::addColumn<QFont::HintingPreference>("hintingPreference");

    QTest::newRow("Default hinting preference") << QFont::PreferDefaultHinting;
    QTest::newRow("No hinting preference") << QFont::PreferNoHinting;
    QTest::newRow("Vertical hinting preference") << QFont::PreferVerticalHinting;
    QTest::newRow("Full hinting preference") << QFont::PreferFullHinting;
}

void tst_QRawFont::rawFontSetPixelSize()
{
    QFETCH(QFont::HintingPreference, hintingPreference);

    QTextLayout layout("Foobar");

    QFont font = layout.font();
    font.setHintingPreference(hintingPreference);
    font.setPixelSize(12);
    layout.setFont(font);

    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    QGlyphRun glyphs = layout.glyphRuns().at(0);
    QRawFont rawFont = glyphs.rawFont();
    QCOMPARE(rawFont.pixelSize(), 12.0);

    rawFont.setPixelSize(24);
    QCOMPARE(rawFont.pixelSize(), 24.0);
}

#if defined(Q_WS_X11) || defined(Q_WS_MAC) && defined(QT_MAC_USE_COCOA)
void tst_QRawFont::multipleRawFontsFromData()
{
    QFile file(QString::fromLatin1(SRCDIR "testfont.ttf"));
    QRawFont testFont;
    if (file.open(QIODevice::ReadOnly)) {
        testFont.loadFromData(file.readAll(), 11, QFont::PreferDefaultHinting);
        file.close();
    }
    file.setFileName(QLatin1String(SRCDIR "testfont_bold_italic.ttf"));
    QRawFont testFontBoldItalic;
    if (file.open(QIODevice::ReadOnly))
        testFontBoldItalic.loadFromData(file.readAll(), 11, QFont::PreferDefaultHinting);

    QVERIFY(testFont.familyName() != (testFontBoldItalic.familyName())
            || testFont.styleName() != (testFontBoldItalic.styleName()));
}
#endif

#endif // QT_NO_RAWFONT

QTEST_MAIN(tst_QRawFont)
#include "tst_qrawfont.moc"

