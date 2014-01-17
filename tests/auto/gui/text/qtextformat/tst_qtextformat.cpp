/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qsettings.h>
#include <qtextformat.h>
#include <private/qtextformat_p.h>
#include <qtextdocument.h>
#include <qtextcursor.h>
#include <qtextobject.h>
#include <qtextlayout.h>
#include <qabstracttextdocumentlayout.h>

class tst_QTextFormat : public QObject
{
Q_OBJECT

private slots:
    void getSetCheck();
    void defaultAlignment();
    void testQTextCharFormat() const;
    void testUnderlinePropertyPrecedence();
    void toFormat();
    void resolveFont();
    void testLetterSpacing();
    void testFontStretch();
    void getSetTabs();
    void testTabsUsed();
    void testFontStyleSetters();
    void setFont_data();
    void setFont();
    void setFont_collection_data();
    void setFont_collection();
};

/*! \internal
  This (used to) trigger a crash in:

    QDataStream &operator>>(QDataStream &stream, QTextFormat &fmt)

  which is most easily produced through QSettings.
 */
void tst_QTextFormat::testQTextCharFormat() const
{
    QSettings settings("test", "test");
    QTextCharFormat test;

    settings.value("test", test);
}

// Testing get/set functions
void tst_QTextFormat::getSetCheck()
{
    QTextFormat obj1;
    // int QTextFormat::objectIndex()
    // void QTextFormat::setObjectIndex(int)
    obj1.setObjectIndex(0);
    QCOMPARE(0, obj1.objectIndex());
    obj1.setObjectIndex(INT_MIN);
    QCOMPARE(INT_MIN, obj1.objectIndex());
    obj1.setObjectIndex(INT_MAX);
    QCOMPARE(INT_MAX, obj1.objectIndex());
}

void tst_QTextFormat::defaultAlignment()
{
    QTextBlockFormat fmt;
    QVERIFY(!fmt.hasProperty(QTextFormat::BlockAlignment));
    QCOMPARE(fmt.intProperty(QTextFormat::BlockAlignment), 0);
    QVERIFY(fmt.alignment() == Qt::AlignLeft);
}

void tst_QTextFormat::testUnderlinePropertyPrecedence()
{
    QTextCharFormat format;
    // use normal accessors and check internal state
    format.setUnderlineStyle(QTextCharFormat::NoUnderline);
    QCOMPARE(format.property(QTextFormat::FontUnderline).isNull(), false);
    QCOMPARE(format.property(QTextFormat::TextUnderlineStyle).isNull(), false);
    QCOMPARE(format.property(QTextFormat::FontUnderline).toBool(), false);
    QCOMPARE(format.property(QTextFormat::TextUnderlineStyle).toInt(), 0);

    format = QTextCharFormat();
    format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
    QCOMPARE(format.property(QTextFormat::FontUnderline).isNull(), false);
    QCOMPARE(format.property(QTextFormat::TextUnderlineStyle).isNull(), false);
    QCOMPARE(format.property(QTextFormat::FontUnderline).toBool(), true);
    QCOMPARE(format.property(QTextFormat::TextUnderlineStyle).toInt(), 1);

    format = QTextCharFormat();
    format.setUnderlineStyle(QTextCharFormat::DotLine);
    QCOMPARE(format.property(QTextFormat::FontUnderline).isNull(), false);
    QCOMPARE(format.property(QTextFormat::TextUnderlineStyle).isNull(), false);
    QCOMPARE(format.property(QTextFormat::FontUnderline).toBool(), false);
    QVERIFY(format.property(QTextFormat::TextUnderlineStyle).toInt() > 0);

    // override accessors and use setProperty to create a false state.
    // then check font()
    format = QTextCharFormat();
    format.setProperty(QTextCharFormat::FontUnderline, true);
    QCOMPARE(format.property(QTextFormat::FontUnderline).isNull(), false);
    QCOMPARE(format.property(QTextFormat::TextUnderlineStyle).isNull(), true);
    QCOMPARE(format.fontUnderline(), true);
    QCOMPARE(format.font().underline(), true);

    format = QTextCharFormat();
    // create conflict. Should use the new property
    format.setProperty(QTextCharFormat::TextUnderlineStyle, QTextCharFormat::SingleUnderline);
    format.setProperty(QTextCharFormat::FontUnderline, false);
    QCOMPARE(format.fontUnderline(), true);
    QCOMPARE(format.font().underline(), true);

    format = QTextCharFormat();
    // create conflict. Should use the new property
    format.setProperty(QTextCharFormat::TextUnderlineStyle, QTextCharFormat::NoUnderline);
    format.setProperty(QTextCharFormat::FontUnderline, true);
    QCOMPARE(format.fontUnderline(), false);
    QCOMPARE(format.font().underline(), false);

    // do it again, but reverse the ordering (we use a QVector internally, so test a LOT ;)
    // create conflict. Should use the new property
    format.setProperty(QTextCharFormat::FontUnderline, false);
    format.setProperty(QTextCharFormat::TextUnderlineStyle, QTextCharFormat::SingleUnderline);
    QCOMPARE(format.fontUnderline(), true);
    QCOMPARE(format.font().underline(), true);

    format = QTextCharFormat();
    // create conflict. Should use the new property
    format.setProperty(QTextCharFormat::FontUnderline, true);
    format.setProperty(QTextCharFormat::TextUnderlineStyle, QTextCharFormat::NoUnderline);
    QCOMPARE(format.fontUnderline(), false);
    QCOMPARE(format.font().underline(), false);
}

void tst_QTextFormat::toFormat()
{
    {
        QTextFormat fmt = QTextFrameFormat();
        QCOMPARE(fmt.toFrameFormat().type(), int(QTextFormat::FrameFormat));
    }

    {
        QTextFormat fmt = QTextTableFormat();
        QCOMPARE(fmt.toTableFormat().type(), int(QTextFormat::FrameFormat));
        QCOMPARE(fmt.toTableFormat().objectType(), int(QTextFormat::TableObject));
    }

    {
        QTextFormat fmt = QTextBlockFormat();
        QCOMPARE(fmt.toBlockFormat().type(), int(QTextFormat::BlockFormat));
    }

    {
        QTextFormat fmt = QTextCharFormat();
        QCOMPARE(fmt.toCharFormat().type(), int(QTextFormat::CharFormat));
    }

    {
        QTextFormat fmt = QTextListFormat();
        QCOMPARE(fmt.toListFormat().type(), int(QTextFormat::ListFormat));
    }
}

void tst_QTextFormat::resolveFont()
{
    QTextDocument doc;

    QTextCharFormat fmt;
    fmt.setProperty(QTextFormat::ForegroundBrush, QColor(Qt::blue));
    QCOMPARE(fmt.property(QTextFormat::ForegroundBrush).userType(), qMetaTypeId<QColor>());
    QCOMPARE(fmt.property(QTextFormat::ForegroundBrush).value<QColor>(), QColor(Qt::blue));
    fmt.setProperty(QTextFormat::FontItalic, true);
    QTextCursor(&doc).insertText("Test", fmt);

    QVector<QTextFormat> formats = doc.allFormats();
    QCOMPARE(formats.count(), 3);

    QVERIFY(formats.at(2).type() == QTextFormat::CharFormat);
    fmt = formats.at(2).toCharFormat();

    QVERIFY(!fmt.font().underline());
    QVERIFY(fmt.hasProperty(QTextFormat::ForegroundBrush));

    QFont f;
    f.setUnderline(true);
    doc.setDefaultFont(f);
    formats = doc.allFormats();
    fmt = formats.at(2).toCharFormat();

    QVERIFY(fmt.font().underline());
    QVERIFY(!fmt.hasProperty(QTextFormat::FontUnderline));

    // verify that deleting a non-existent property does not affect the font resolving

    QVERIFY(!fmt.hasProperty(QTextFormat::BackgroundBrush));
    fmt.clearProperty(QTextFormat::BackgroundBrush);
    QVERIFY(!fmt.hasProperty(QTextFormat::BackgroundBrush));

    QVERIFY(!fmt.hasProperty(QTextFormat::FontUnderline));
    QVERIFY(fmt.font().underline());

    // verify that deleting an existent but font _unrelated_ property does not affect the font resolving

    QVERIFY(fmt.hasProperty(QTextFormat::ForegroundBrush));
    fmt.clearProperty(QTextFormat::ForegroundBrush);
    QVERIFY(!fmt.hasProperty(QTextFormat::ForegroundBrush));

    QVERIFY(!fmt.hasProperty(QTextFormat::FontUnderline));
    QVERIFY(fmt.font().underline());

    // verify that removing a font property _does_ clear the resolving

    QVERIFY(fmt.hasProperty(QTextFormat::FontItalic));
    fmt.clearProperty(QTextFormat::FontItalic);
    QVERIFY(!fmt.hasProperty(QTextFormat::FontItalic));

    QVERIFY(!fmt.hasProperty(QTextFormat::FontUnderline));
    QVERIFY(!fmt.font().underline());
    QVERIFY(!fmt.font().italic());

    // reset
    fmt = formats.at(2).toCharFormat();

    QVERIFY(fmt.font().underline());
    QVERIFY(!fmt.hasProperty(QTextFormat::FontUnderline));

    // verify that _setting_ an unrelated property does _not_ affect the resolving

    QVERIFY(!fmt.hasProperty(QTextFormat::IsAnchor));
    fmt.setProperty(QTextFormat::IsAnchor, true);
    QVERIFY(fmt.hasProperty(QTextFormat::IsAnchor));

    QVERIFY(fmt.font().underline());
    QVERIFY(!fmt.hasProperty(QTextFormat::FontUnderline));

    // verify that setting a _related_ font property does affect the resolving
    //
    QVERIFY(!fmt.hasProperty(QTextFormat::FontStrikeOut));
    fmt.setProperty(QTextFormat::FontStrikeOut, true);
    QVERIFY(fmt.hasProperty(QTextFormat::FontStrikeOut));

    QVERIFY(!fmt.font().underline());
    QVERIFY(!fmt.hasProperty(QTextFormat::FontUnderline));
    QVERIFY(fmt.font().strikeOut());
}


void tst_QTextFormat::testLetterSpacing()
{
    QTextCharFormat format;

    QCOMPARE(format.hasProperty(QTextFormat::FontLetterSpacing), false);
    QCOMPARE(format.hasProperty(QTextFormat::FontLetterSpacingType), false);

    format.setFontLetterSpacingType(QFont::AbsoluteSpacing);
    format.setFontLetterSpacing(10.0);

    QCOMPARE(format.hasProperty(QTextFormat::FontLetterSpacing), true);
    QCOMPARE(format.property(QTextFormat::FontLetterSpacing).toDouble(), 10.0);
    QCOMPARE(format.property(QTextFormat::FontLetterSpacingType).toInt(), int(QFont::AbsoluteSpacing));

    format.setFontLetterSpacingType(QFont::PercentageSpacing);
    format.setFontLetterSpacing(110.0);

    QCOMPARE(format.property(QTextFormat::FontLetterSpacing).toDouble(), 110.0);
    QCOMPARE(format.property(QTextFormat::FontLetterSpacingType).toInt(), int(QFont::PercentageSpacing));

    format.setFontLetterSpacingType(QFont::AbsoluteSpacing);
    format.setFontLetterSpacing(10.0);

    QCOMPARE(format.property(QTextFormat::FontLetterSpacingType).toInt(), int(QFont::AbsoluteSpacing));
    QCOMPARE(format.property(QTextFormat::FontLetterSpacing).toDouble(), 10.0);
}

void tst_QTextFormat::testFontStretch()
{
    QTextCharFormat format;

    QCOMPARE(format.hasProperty(QTextFormat::FontStretch), false);

    format.setFontStretch(130.0);

    QCOMPARE(format.property(QTextFormat::FontStretch).toInt(), 130);
}

void tst_QTextFormat::getSetTabs()
{
    class Comparator {
      public:
        Comparator(const QList<QTextOption::Tab> &tabs, const QList<QTextOption::Tab> &tabs2)
        {
            QCOMPARE(tabs.count(), tabs2.count());
            for(int i=0; i < tabs.count(); i++) {
                QTextOption::Tab t1 = tabs[i];
                QTextOption::Tab t2 = tabs2[i];
                QCOMPARE(t1.position, t2.position);
                QCOMPARE(t1.type, t2.type);
                QCOMPARE(t1.delimiter, t2.delimiter);
            }
        }
    };

    QList<QTextOption::Tab> tabs;
    QTextBlockFormat format;
    format.setTabPositions(tabs);
    Comparator c1(tabs, format.tabPositions());

    QTextOption::Tab tab1;
    tab1.position = 1234;
    tabs.append(tab1);
    format.setTabPositions(tabs);
    Comparator c2(tabs, format.tabPositions());

    QTextOption::Tab tab2(3456, QTextOption::RightTab, QChar('x'));
    tabs.append(tab2);
    format.setTabPositions(tabs);
    Comparator c3(tabs, format.tabPositions());
}

void tst_QTextFormat::testTabsUsed()
{
    QTextDocument doc;
    QTextCursor cursor(&doc);

    QList<QTextOption::Tab> tabs;
    QTextBlockFormat format;
    QTextOption::Tab tab;
    tab.position = 100;
    tabs.append(tab);
    format.setTabPositions(tabs);
    cursor.mergeBlockFormat(format);
    cursor.insertText("foo\tbar");
    //doc.setPageSize(QSizeF(200, 200));
    doc.documentLayout()->pageCount(); // force layout;

    QTextBlock block = doc.begin();
    QTextLayout *layout = block.layout();
    QVERIFY(layout);
    QCOMPARE(layout->lineCount(), 1);
    QTextLine line = layout->lineAt(0);
    QCOMPARE(line.cursorToX(4), 100.);

    QTextOption option = layout->textOption();
    QCOMPARE(option.tabs().count(), tabs.count());

}

void tst_QTextFormat::testFontStyleSetters()
{
    QTextCharFormat format;

    // test the setters
    format.setFontStyleHint(QFont::Serif);
    QCOMPARE(format.font().styleHint(), QFont::Serif);
    QCOMPARE(format.font().styleStrategy(), QFont::PreferDefault);
    format.setFontStyleStrategy(QFont::PreferOutline);
    QCOMPARE(format.font().styleStrategy(), QFont::PreferOutline);

    // test setting properties through setFont()
    QFont font;
    font.setStyleHint(QFont::SansSerif, QFont::PreferAntialias);
    format.setFont(font);
    QCOMPARE(format.font().styleHint(), QFont::SansSerif);
    QCOMPARE(format.font().styleStrategy(), QFont::PreferAntialias);

    // test kerning
    format.setFontKerning(false);
    QCOMPARE(format.font().kerning(), false);
    format.setFontKerning(true);
    QCOMPARE(format.font().kerning(), true);
    font.setKerning(false);
    format.setFont(font);
    QCOMPARE(format.font().kerning(), false);
}

Q_DECLARE_METATYPE(QTextCharFormat)

void tst_QTextFormat::setFont_data()
{
    QTest::addColumn<QTextCharFormat>("format1");
    QTest::addColumn<QTextCharFormat>("format2");
    QTest::addColumn<bool>("overrideAll");

    QTextCharFormat format1;
    format1.setFontStyleHint(QFont::Serif);
    format1.setFontStyleStrategy(QFont::PreferOutline);
    format1.setFontCapitalization(QFont::AllUppercase);
    format1.setFontKerning(true);

    {
        QTest::newRow("noop|override") << format1 << format1 << true;
        QTest::newRow("noop|inherit") << format1 << format1 << false;
    }

    {
        QTextCharFormat format2;
        format2.setFontStyleHint(QFont::SansSerif);
        format2.setFontStyleStrategy(QFont::PreferAntialias);
        format2.setFontCapitalization(QFont::MixedCase);
        format2.setFontKerning(false);

        QTest::newRow("coverage|override") << format1 << format2 << true;
        QTest::newRow("coverage|inherit") << format1 << format2 << false;
    }

    {
        QTextCharFormat format2;
        format2.setFontStyleHint(QFont::SansSerif);
        format2.setFontStyleStrategy(QFont::PreferAntialias);

        QTest::newRow("partial|override") << format1 << format2 << true;
        QTest::newRow("partial|inherit") << format1 << format2 << false;
    }
}

void tst_QTextFormat::setFont()
{
    QFETCH(QTextCharFormat, format1);
    QFETCH(QTextCharFormat, format2);
    QFETCH(bool, overrideAll);

    QTextCharFormat f;

    f.merge(format1);
    QCOMPARE((int)f.fontStyleHint(), (int)format1.fontStyleHint());
    QCOMPARE((int)f.fontStyleStrategy(), (int)format1.fontStyleStrategy());
    QCOMPARE((int)f.fontCapitalization(), (int)format1.fontCapitalization());
    QCOMPARE(f.fontKerning(), format1.fontKerning());

    QCOMPARE((int)f.font().styleHint(), (int)f.fontStyleHint());
    QCOMPARE((int)f.font().styleStrategy(), (int)f.fontStyleStrategy());
    QCOMPARE((int)f.font().capitalization(), (int)f.fontCapitalization());
    QCOMPARE(f.font().kerning(), f.fontKerning());

    f.merge(format2);
    QCOMPARE((int)f.font().styleHint(), (int)f.fontStyleHint());
    QCOMPARE((int)f.font().styleStrategy(), (int)f.fontStyleStrategy());
    QCOMPARE((int)f.font().capitalization(), (int)f.fontCapitalization());
    QCOMPARE(f.font().kerning(), f.fontKerning());

    if (format2.hasProperty(QTextFormat::FontStyleHint))
        QCOMPARE((int)f.font().styleHint(), (int)format2.fontStyleHint());
    else
        QCOMPARE((int)f.font().styleHint(), (int)format1.fontStyleHint());
    if (format2.hasProperty(QTextFormat::FontStyleStrategy))
        QCOMPARE((int)f.font().styleStrategy(), (int)format2.fontStyleStrategy());
    else
        QCOMPARE((int)f.font().styleStrategy(), (int)format1.fontStyleStrategy());
    if (format2.hasProperty(QTextFormat::FontCapitalization))
        QCOMPARE((int)f.font().capitalization(), (int)format2.fontCapitalization());
    else
        QCOMPARE((int)f.font().capitalization(), (int)format1.fontCapitalization());
    if (format2.hasProperty(QTextFormat::FontKerning))
        QCOMPARE(f.font().kerning(), format2.fontKerning());
    else
        QCOMPARE(f.font().kerning(), format1.fontKerning());


    QFont font1 = format1.font();
    QFont font2 = format2.font();

    f = QTextCharFormat();

    {
        QTextCharFormat tmp;
        tmp.setFont(font1, overrideAll ? QTextCharFormat::FontPropertiesAll
                                       : QTextCharFormat::FontPropertiesSpecifiedOnly);
        f.merge(tmp);
    }
    QCOMPARE((int)f.fontStyleHint(), (int)format1.fontStyleHint());
    QCOMPARE((int)f.fontStyleStrategy(), (int)format1.fontStyleStrategy());
    QCOMPARE((int)f.fontCapitalization(), (int)format1.fontCapitalization());
    QCOMPARE(f.fontKerning(), format1.fontKerning());

    QCOMPARE((int)f.font().styleHint(), (int)f.fontStyleHint());
    QCOMPARE((int)f.font().styleStrategy(), (int)f.fontStyleStrategy());
    QCOMPARE((int)f.font().capitalization(), (int)f.fontCapitalization());
    QCOMPARE(f.font().kerning(), f.fontKerning());

    {
        QTextCharFormat tmp;
        tmp.setFont(font2, overrideAll ? QTextCharFormat::FontPropertiesAll
                                       : QTextCharFormat::FontPropertiesSpecifiedOnly);
        f.merge(tmp);
    }
    QCOMPARE((int)f.font().styleHint(), (int)f.fontStyleHint());
    QCOMPARE((int)f.font().styleStrategy(), (int)f.fontStyleStrategy());
    QCOMPARE((int)f.font().capitalization(), (int)f.fontCapitalization());
    QCOMPARE(f.font().kerning(), f.fontKerning());

    if (overrideAll || (font2.resolve() & QFont::StyleHintResolved))
        QCOMPARE((int)f.font().styleHint(), (int)font2.styleHint());
    else
        QCOMPARE((int)f.font().styleHint(), (int)font1.styleHint());
    if (overrideAll || (font2.resolve() & QFont::StyleStrategyResolved))
        QCOMPARE((int)f.font().styleStrategy(), (int)font2.styleStrategy());
    else
        QCOMPARE((int)f.font().styleStrategy(), (int)font1.styleStrategy());
    if (overrideAll || (font2.resolve() & QFont::CapitalizationResolved))
        QCOMPARE((int)f.font().capitalization(), (int)font2.capitalization());
    else
        QCOMPARE((int)f.font().capitalization(), (int)font1.capitalization());
    if (overrideAll || (font2.resolve() & QFont::KerningResolved))
        QCOMPARE(f.font().kerning(), font2.kerning());
    else
        QCOMPARE(f.font().kerning(), font1.kerning());
}

void tst_QTextFormat::setFont_collection_data()
{
    setFont_data();
}

void tst_QTextFormat::setFont_collection()
{
    QFETCH(QTextCharFormat, format1);
    QFETCH(QTextCharFormat, format2);
    QFETCH(bool, overrideAll);

    QFont font1 = format1.font();
    QFont font2 = format2.font();

    {
        QTextFormatCollection collection;
        collection.setDefaultFont(font1);

        int formatIndex = collection.indexForFormat(format1);
        QTextCharFormat f = collection.charFormat(formatIndex);

        QCOMPARE((int)f.fontStyleHint(), (int)format1.fontStyleHint());
        QCOMPARE((int)f.fontStyleStrategy(), (int)format1.fontStyleStrategy());
        QCOMPARE((int)f.fontCapitalization(), (int)format1.fontCapitalization());
        QCOMPARE(f.fontKerning(), format1.fontKerning());

        QCOMPARE((int)f.font().styleHint(), (int)f.fontStyleHint());
        QCOMPARE((int)f.font().styleStrategy(), (int)f.fontStyleStrategy());
        QCOMPARE((int)f.font().capitalization(), (int)f.fontCapitalization());
        QCOMPARE(f.font().kerning(), f.fontKerning());
    }
    {
        QTextFormatCollection collection;
        collection.setDefaultFont(font1);

        int formatIndex = collection.indexForFormat(format2);
        QTextCharFormat f = collection.charFormat(formatIndex);

        if (format2.hasProperty(QTextFormat::FontStyleHint))
            QCOMPARE((int)f.font().styleHint(), (int)format2.fontStyleHint());
        else
            QCOMPARE((int)f.font().styleHint(), (int)format1.fontStyleHint());
        if (format2.hasProperty(QTextFormat::FontStyleStrategy))
            QCOMPARE((int)f.font().styleStrategy(), (int)format2.fontStyleStrategy());
        else
            QCOMPARE((int)f.font().styleStrategy(), (int)format1.fontStyleStrategy());
        if (format2.hasProperty(QTextFormat::FontCapitalization))
            QCOMPARE((int)f.font().capitalization(), (int)format2.fontCapitalization());
        else
            QCOMPARE((int)f.font().capitalization(), (int)format1.fontCapitalization());
        if (format2.hasProperty(QTextFormat::FontKerning))
            QCOMPARE(f.font().kerning(), format2.fontKerning());
        else
            QCOMPARE(f.font().kerning(), format1.fontKerning());
    }

    {
        QTextFormatCollection collection;
        collection.setDefaultFont(font1);

        QTextCharFormat tmp;
        tmp.setFont(font1, overrideAll ? QTextCharFormat::FontPropertiesAll
                                       : QTextCharFormat::FontPropertiesSpecifiedOnly);
        int formatIndex = collection.indexForFormat(tmp);
        QTextCharFormat f = collection.charFormat(formatIndex);

        QCOMPARE((int)f.fontStyleHint(), (int)format1.fontStyleHint());
        QCOMPARE((int)f.fontStyleStrategy(), (int)format1.fontStyleStrategy());
        QCOMPARE((int)f.fontCapitalization(), (int)format1.fontCapitalization());
        QCOMPARE(f.fontKerning(), format1.fontKerning());

        QCOMPARE((int)f.font().styleHint(), (int)f.fontStyleHint());
        QCOMPARE((int)f.font().styleStrategy(), (int)f.fontStyleStrategy());
        QCOMPARE((int)f.font().capitalization(), (int)f.fontCapitalization());
        QCOMPARE(f.font().kerning(), f.fontKerning());
    }
    {
        QTextFormatCollection collection;
        collection.setDefaultFont(font1);

        QTextCharFormat tmp;
        tmp.setFont(font2, overrideAll ? QTextCharFormat::FontPropertiesAll
                                       : QTextCharFormat::FontPropertiesSpecifiedOnly);
        int formatIndex = collection.indexForFormat(tmp);
        QTextCharFormat f = collection.charFormat(formatIndex);

        if (overrideAll || (font2.resolve() & QFont::StyleHintResolved))
            QCOMPARE((int)f.font().styleHint(), (int)font2.styleHint());
        else
            QCOMPARE((int)f.font().styleHint(), (int)font1.styleHint());
        if (overrideAll || (font2.resolve() & QFont::StyleStrategyResolved))
            QCOMPARE((int)f.font().styleStrategy(), (int)font2.styleStrategy());
        else
            QCOMPARE((int)f.font().styleStrategy(), (int)font1.styleStrategy());
        if (overrideAll || (font2.resolve() & QFont::CapitalizationResolved))
            QCOMPARE((int)f.font().capitalization(), (int)font2.capitalization());
        else
            QCOMPARE((int)f.font().capitalization(), (int)font1.capitalization());
        if (overrideAll || (font2.resolve() & QFont::KerningResolved))
            QCOMPARE(f.font().kerning(), font2.kerning());
        else
            QCOMPARE(f.font().kerning(), font1.kerning());
    }
}

QTEST_MAIN(tst_QTextFormat)
#include "tst_qtextformat.moc"
