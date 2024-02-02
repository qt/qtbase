// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>
#include <QSignalSpy>
#include <QFontDatabase>

#include <qfontcombobox.h>

class tst_QFontComboBox : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void qfontcombobox_data();
    void qfontcombobox();
    void currentFont_data();
    void currentFont();
    void fontFilters_data();
    void fontFilters();
    void sizeHint();
    void writingSystem_data();
    void writingSystem();
    void currentFontChanged();
    void emptyFont();
};

// Subclass that exposes the protected functions.
class SubQFontComboBox : public QFontComboBox
{
public:
    void call_currentFontChanged(QFont const& f)
        { return SubQFontComboBox::currentFontChanged(f); }

    bool call_event(QEvent* e)
        { return SubQFontComboBox::event(e); }
};

void tst_QFontComboBox::initTestCase()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This freezes. Figure out why.");
}

void tst_QFontComboBox::qfontcombobox_data()
{
}

void tst_QFontComboBox::qfontcombobox()
{
    SubQFontComboBox box;
    QCOMPARE(box.currentFont(), QFont());
    QCOMPARE(box.fontFilters(), QFontComboBox::AllFonts);
    box.setCurrentFont(QFont());
    box.setFontFilters(QFontComboBox::AllFonts);
    box.setWritingSystem(QFontDatabase::Any);
    QVERIFY(box.sizeHint() != QSize());
    QCOMPARE(box.writingSystem(), QFontDatabase::Any);
    box.call_currentFontChanged(QFont());
    QEvent event(QEvent::None);
    QCOMPARE(box.call_event(&event), false);
}

void tst_QFontComboBox::currentFont_data()
{
    QTest::addColumn<QFont>("currentFont");
    // Normalize the names
    QFont defaultFont;
    QFontInfo fi(defaultFont);
    defaultFont = QFont(QStringList{fi.family()}); // make sure we have a real font name and not something like 'Sans Serif'.
    if (!QFontDatabase::isPrivateFamily(defaultFont.family()))
        QTest::newRow("default") << defaultFont;
    defaultFont.setPointSize(defaultFont.pointSize() + 10);
    if (!QFontDatabase::isPrivateFamily(defaultFont.family()))
        QTest::newRow("default2") << defaultFont;
    QStringList list = QFontDatabase::families();
    for (int i = 0; i < list.size(); ++i) {
        QFont f = QFont(QStringList{QFontInfo(QFont(list.at(i))).family()});
        if (!QFontDatabase::isPrivateFamily(f.families().first()))
            QTest::newRow(qPrintable(list.at(i))) << f;
    }
}

// public QFont currentFont() const
void tst_QFontComboBox::currentFont()
{
    QFETCH(QFont, currentFont);

    SubQFontComboBox box;
    QSignalSpy spy0(&box, SIGNAL(currentFontChanged(QFont)));
    QFont oldCurrentFont = box.currentFont();

    box.setCurrentFont(currentFont);
    QRegularExpression foundry(" \\[.*\\]");
    if (!box.currentFont().family().contains(foundry)) {
        QCOMPARE(box.currentFont(), currentFont);
    }
    QString boxFontFamily = QFontInfo(box.currentFont()).family();
    if (!currentFont.family().contains(foundry))
        boxFontFamily.remove(foundry);
    QCOMPARE(boxFontFamily, currentFont.family());

    if (oldCurrentFont != box.currentFont()) {
        //the signal may be emit twice if there is a foundry into brackets
        QCOMPARE(spy0.size(),1);
    }
}

Q_DECLARE_METATYPE(QFontComboBox::FontFilters)
void tst_QFontComboBox::fontFilters_data()
{
    QTest::addColumn<QFontComboBox::FontFilters>("fontFilters");
    QTest::newRow("AllFonts")
        << QFontComboBox::FontFilters(QFontComboBox::AllFonts);
    QTest::newRow("ScalableFonts")
        << QFontComboBox::FontFilters(QFontComboBox::ScalableFonts);
    QTest::newRow("NonScalableFonts")
        << QFontComboBox::FontFilters(QFontComboBox::NonScalableFonts);
    QTest::newRow("MonospacedFonts")
        << QFontComboBox::FontFilters(QFontComboBox::MonospacedFonts);
    QTest::newRow("ProportionalFonts")
        << QFontComboBox::FontFilters(QFontComboBox::ProportionalFonts);

    // combine two
    QTest::newRow("ProportionalFonts | NonScalableFonts")
        << QFontComboBox::FontFilters(QFontComboBox::ProportionalFonts | QFontComboBox::NonScalableFonts);

    // i.e. all
    QTest::newRow("ScalableFonts | NonScalableFonts")
        << QFontComboBox::FontFilters(QFontComboBox::ScalableFonts | QFontComboBox::NonScalableFonts);

}

// public QFontComboBox::FontFilters fontFilters() const
void tst_QFontComboBox::fontFilters()
{
    QFETCH(QFontComboBox::FontFilters, fontFilters);

    SubQFontComboBox box;
    QSignalSpy spy0(&box, SIGNAL(currentFontChanged(QFont)));
    QFont currentFont = box.currentFont();

    box.setFontFilters(fontFilters);
    QCOMPARE(box.fontFilters(), fontFilters);

    QStringList list = QFontDatabase::families();
    int c = 0;
    const int scalableMask = (QFontComboBox::ScalableFonts | QFontComboBox::NonScalableFonts);
    const int spacingMask = (QFontComboBox::ProportionalFonts | QFontComboBox::MonospacedFonts);
    if((fontFilters & scalableMask) == scalableMask)
        fontFilters &= ~scalableMask;
    if((fontFilters & spacingMask) == spacingMask)
        fontFilters &= ~spacingMask;

    for (int i = 0; i < list.size(); ++i) {
        if (QFontDatabase::isPrivateFamily(list[i]))
            continue;
        if (fontFilters & QFontComboBox::ScalableFonts) {
            if (!QFontDatabase::isSmoothlyScalable(list[i]))
                continue;
        } else if (fontFilters & QFontComboBox::NonScalableFonts) {
            if (QFontDatabase::isSmoothlyScalable(list[i]))
                continue;
        }
        if (fontFilters & QFontComboBox::MonospacedFonts) {
            if (!QFontDatabase::isFixedPitch(list[i]))
                continue;
        } else if (fontFilters & QFontComboBox::ProportionalFonts) {
            if (QFontDatabase::isFixedPitch(list[i]))
                continue;
        }
        c++;
    }

    QCOMPARE(box.model()->rowCount(), c);

    if (c == 0)
        QCOMPARE(box.currentFont(), QFont());

    QCOMPARE(spy0.size(), (currentFont != box.currentFont()) ? 1 : 0);
}

// public QSize sizeHint() const
void tst_QFontComboBox::sizeHint()
{
    SubQFontComboBox box;
    QSize sizeHint = box.QComboBox::sizeHint();
    QFontMetrics fm(box.font());
    sizeHint.setWidth(qMax(sizeHint.width(), fm.horizontalAdvance(QLatin1Char('m'))*14));
    QCOMPARE(box.sizeHint(), sizeHint);
}

Q_DECLARE_METATYPE(QFontDatabase::WritingSystem)
void tst_QFontComboBox::writingSystem_data()
{
    QTest::addColumn<QFontDatabase::WritingSystem>("writingSystem");
    QTest::newRow("Any") << QFontDatabase::Any;
    QTest::newRow("Latin") << QFontDatabase::Latin;
    QTest::newRow("Lao") << QFontDatabase::Lao;
    QTest::newRow("TraditionalChinese") << QFontDatabase::TraditionalChinese;
    QTest::newRow("Ogham") << QFontDatabase::Ogham;
    QTest::newRow("Runic") << QFontDatabase::Runic;

    for (int i = 0; i < 31; ++i)
        QTest::newRow(("enum " + QByteArray::number(i)).constData()) << (QFontDatabase::WritingSystem)i;
}

// public QFontDatabase::WritingSystem writingSystem() const
void tst_QFontComboBox::writingSystem()
{
    QFETCH(QFontDatabase::WritingSystem, writingSystem);

    SubQFontComboBox box;
    QSignalSpy spy0(&box, SIGNAL(currentFontChanged(QFont)));
    QFont currentFont = box.currentFont();

    box.setWritingSystem(writingSystem);
    QCOMPARE(box.writingSystem(), writingSystem);

    QStringList list = QFontDatabase::families(writingSystem);
    int c = list.size();
    for (int i = 0; i < list.size(); ++i) {
        if (QFontDatabase::isPrivateFamily(list[i]))
            c--;
    }
    QCOMPARE(box.model()->rowCount(), c);

    if (list.size() == 0)
        QCOMPARE(box.currentFont(), QFont());

    QCOMPARE(spy0.size(), (currentFont != box.currentFont()) ? 1 : 0);
}

// protected void currentFontChanged(QFont const& f)
void tst_QFontComboBox::currentFontChanged()
{
    // The absence of this file does not affect the test results
    QFontDatabase::addApplicationFont("ArianaVioleta-dz2K.ttf");

    SubQFontComboBox *box = new SubQFontComboBox;
    QSignalSpy spy0(box, SIGNAL(currentFontChanged(QFont)));

    if (box->model()->rowCount() > 2) {
        QTest::keyPress(box, Qt::Key_Down);
        QCOMPARE(spy0.size(), 1);

        QFont f( "Sans Serif" );
        box->setCurrentFont(f);
        QCOMPARE(spy0.size(), 2);
    } else
        qWarning("Not enough fonts installed on test system. Consider adding some");
}

void tst_QFontComboBox::emptyFont()
{
    QFontComboBox fontCB;
    if (fontCB.count() < 2)
        QSKIP("Not enough fonts on system to run test.");

    QFont font;
    font.setFamilies(QStringList());

    // Due to QTBUG-98341, we need to find an index in the family list
    // which does not match the default index for the empty font, otherwise
    // the font selection will not be properly updated.
    {
        QFontInfo fi(font);
        QStringList families = QFontDatabase::families();
        int index = families.indexOf(fi.family());
        if (index < 0)
            index = 0;
        if (index > 0)
            index--;
        else
            index++;

        fontCB.setCurrentIndex(index);
    }

    fontCB.setCurrentFont(font);
    QVERIFY(!fontCB.currentFont().families().isEmpty());
}

QTEST_MAIN(tst_QFontComboBox)
#include "tst_qfontcombobox.moc"

