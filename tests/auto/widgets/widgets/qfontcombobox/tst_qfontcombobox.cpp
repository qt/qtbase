/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include <qfontcombobox.h>

class tst_QFontComboBox : public QObject
{
    Q_OBJECT

private slots:
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
    QFontDatabase db;
    // Normalize the names
    QFont defaultFont;
    QFontInfo fi(defaultFont);
    defaultFont = QFont(fi.family()); // make sure we have a real font name and not something like 'Sans Serif'.
    if (!db.isPrivateFamily(defaultFont.family()))
        QTest::newRow("default") << defaultFont;
    defaultFont.setPointSize(defaultFont.pointSize() + 10);
    if (!db.isPrivateFamily(defaultFont.family()))
        QTest::newRow("default2") << defaultFont;
    QStringList list = db.families();
    for (int i = 0; i < list.count(); ++i) {
        QFont f = QFont(QFontInfo(QFont(list.at(i))).family());
        if (!db.isPrivateFamily(f.family()))
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
    QRegExp foundry(" \\[.*\\]");
    if (!box.currentFont().family().contains(foundry)) {
        QCOMPARE(box.currentFont(), currentFont);
    }
    QString boxFontFamily = QFontInfo(box.currentFont()).family();
    if (!currentFont.family().contains(foundry))
        boxFontFamily.remove(foundry);
    QCOMPARE(boxFontFamily, currentFont.family());

    if (oldCurrentFont != box.currentFont()) {
        //the signal may be emit twice if there is a foundry into brackets
        QCOMPARE(spy0.count(),1);
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

    QFontDatabase db;
    QStringList list = db.families();
    int c = 0;
    const int scalableMask = (QFontComboBox::ScalableFonts | QFontComboBox::NonScalableFonts);
    const int spacingMask = (QFontComboBox::ProportionalFonts | QFontComboBox::MonospacedFonts);
    if((fontFilters & scalableMask) == scalableMask)
        fontFilters &= ~scalableMask;
    if((fontFilters & spacingMask) == spacingMask)
        fontFilters &= ~spacingMask;

    for (int i = 0; i < list.count(); ++i) {
        if (db.isPrivateFamily(list[i]))
            continue;
        if (fontFilters & QFontComboBox::ScalableFonts) {
            if (!db.isSmoothlyScalable(list[i]))
                continue;
        } else if (fontFilters & QFontComboBox::NonScalableFonts) {
            if (db.isSmoothlyScalable(list[i]))
                continue;
        }
        if (fontFilters & QFontComboBox::MonospacedFonts) {
            if (!db.isFixedPitch(list[i]))
                continue;
        } else if (fontFilters & QFontComboBox::ProportionalFonts) {
            if (db.isFixedPitch(list[i]))
                continue;
        }
        c++;
    }

    QCOMPARE(box.model()->rowCount(), c);

    if (c == 0)
        QCOMPARE(box.currentFont(), QFont());

    QCOMPARE(spy0.count(), (currentFont != box.currentFont()) ? 1 : 0);
}

// public QSize sizeHint() const
void tst_QFontComboBox::sizeHint()
{
    SubQFontComboBox box;
    QSize sizeHint = box.QComboBox::sizeHint();
    QFontMetrics fm(box.font());
    sizeHint.setWidth(qMax(sizeHint.width(), fm.width(QLatin1Char('m'))*14));
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

    QFontDatabase db;
    QStringList list = db.families(writingSystem);
    int c = list.count();
    for (int i = 0; i < list.count(); ++i) {
        if (db.isPrivateFamily(list[i]))
            c--;
    }
    QCOMPARE(box.model()->rowCount(), c);

    if (list.count() == 0)
        QCOMPARE(box.currentFont(), QFont());

    QCOMPARE(spy0.count(), (currentFont != box.currentFont()) ? 1 : 0);
}

// protected void currentFontChanged(QFont const& f)
void tst_QFontComboBox::currentFontChanged()
{
    SubQFontComboBox box;
    QSignalSpy spy0(&box, SIGNAL(currentFontChanged(QFont)));

    if (box.model()->rowCount() > 2) {
        QTest::keyPress(&box, Qt::Key_Down);
        QCOMPARE(spy0.count(), 1);

        QFont f( "Sans Serif" );
        box.setCurrentFont(f);
        QCOMPARE(spy0.count(), 2);
    } else
        qWarning("Not enough fonts installed on test system. Consider adding some");
}

QTEST_MAIN(tst_QFontComboBox)
#include "tst_qfontcombobox.moc"

