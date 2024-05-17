// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>

#include "qpalette.h"

class tst_QPalette : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void roleValues_data();
    void roleValues();
    void resolve();
    void copySemantics();
    void moveSemantics();
    void setBrush();

    void isBrushSet();
    void setAllPossibleBrushes();
    void noBrushesSetForDefaultPalette();
    void cannotCheckIfInvalidBrushSet();
    void checkIfBrushForCurrentGroupSet();
    void cacheKey();
    void dataStream();
};

void tst_QPalette::roleValues_data()
{
    QTest::addColumn<int>("role");
    QTest::addColumn<int>("value");

    QTest::newRow("QPalette::WindowText") << int(QPalette::WindowText) << 0;
    QTest::newRow("QPalette::Button") << int(QPalette::Button) << 1;
    QTest::newRow("QPalette::Light") << int(QPalette::Light) << 2;
    QTest::newRow("QPalette::Midlight") << int(QPalette::Midlight) << 3;
    QTest::newRow("QPalette::Dark") << int(QPalette::Dark) << 4;
    QTest::newRow("QPalette::Mid") << int(QPalette::Mid) << 5;
    QTest::newRow("QPalette::Text") << int(QPalette::Text) << 6;
    QTest::newRow("QPalette::BrightText") << int(QPalette::BrightText) << 7;
    QTest::newRow("QPalette::ButtonText") << int(QPalette::ButtonText) << 8;
    QTest::newRow("QPalette::Base") << int(QPalette::Base) << 9;
    QTest::newRow("QPalette::Window") << int(QPalette::Window) << 10;
    QTest::newRow("QPalette::Shadow") << int(QPalette::Shadow) << 11;
    QTest::newRow("QPalette::Highlight") << int(QPalette::Highlight) << 12;
    QTest::newRow("QPalette::HighlightedText") << int(QPalette::HighlightedText) << 13;
    QTest::newRow("QPalette::Link") << int(QPalette::Link) << 14;
    QTest::newRow("QPalette::LinkVisited") << int(QPalette::LinkVisited) << 15;
    QTest::newRow("QPalette::AlternateBase") << int(QPalette::AlternateBase) << 16;
    QTest::newRow("QPalette::NoRole") << int(QPalette::NoRole) << 17;
    QTest::newRow("QPalette::ToolTipBase") << int(QPalette::ToolTipBase) << 18;
    QTest::newRow("QPalette::ToolTipText") << int(QPalette::ToolTipText) << 19;
    QTest::newRow("QPalette::PlaceholderText") << int(QPalette::PlaceholderText) << 20;
    QTest::newRow("QPalette::Accent") << int(QPalette::Accent) << 21;

    // Change this value as you add more roles.
    QTest::newRow("QPalette::NColorRoles") << int(QPalette::NColorRoles) << 22;
}

void tst_QPalette::roleValues()
{
    QFETCH(int, role);
    QFETCH(int, value);
    QCOMPARE(role, value);
}

void tst_QPalette::resolve()
{
    QPalette p1;
    p1.setBrush(QPalette::WindowText, Qt::green);
    p1.setBrush(QPalette::Button, Qt::green);

    QVERIFY(p1.isBrushSet(QPalette::Active, QPalette::WindowText));
    QVERIFY(p1.isBrushSet(QPalette::Active, QPalette::Button));

    QPalette p2;
    p2.setBrush(QPalette::WindowText, Qt::red);

    QVERIFY(p2.isBrushSet(QPalette::Active, QPalette::WindowText));
    QVERIFY(!p2.isBrushSet(QPalette::Active, QPalette::Button));

    QPalette p1ResolvedTo2 = p1.resolve(p2);
    // p1ResolvedTo2 gets everything from p1 and nothing copied from p2 because
    // it already has a WindowText. That is two brushes, and to the same value
    // as p1.
    QCOMPARE(p1ResolvedTo2, p1);
    QVERIFY(p1ResolvedTo2.isBrushSet(QPalette::Active, QPalette::WindowText));
    QCOMPARE(p1.windowText(), p1ResolvedTo2.windowText());
    QVERIFY(p1ResolvedTo2.isBrushSet(QPalette::Active, QPalette::Button));
    QCOMPARE(p1.button(), p1ResolvedTo2.button());

    QPalette p2ResolvedTo1 = p2.resolve(p1);
    // p2ResolvedTo1 gets the WindowText set, and to the same value as the
    // original p2, however, Button gets set from p1.
    QVERIFY(p2ResolvedTo1.isBrushSet(QPalette::Active, QPalette::WindowText));
    QCOMPARE(p2.windowText(), p2ResolvedTo1.windowText());
    QVERIFY(p2ResolvedTo1.isBrushSet(QPalette::Active, QPalette::Button));
    QCOMPARE(p1.button(), p2ResolvedTo1.button());

    QVERIFY(p2ResolvedTo1 != p1);
    QVERIFY(p2ResolvedTo1 != p2);

    QPalette p3;
    // ensure the resolve mask is full
    for (int r = 0; r < QPalette::NColorRoles; ++r)
        p3.setBrush(QPalette::All, QPalette::ColorRole(r), Qt::red);
    const QPalette::ResolveMask fullMask = p3.resolveMask();

    QPalette p3ResolvedToP1 = p3.resolve(p1);
    QVERIFY(p3ResolvedToP1.isCopyOf(p3));

    QPalette p4;
    QCOMPARE(p4.resolveMask(), QPalette::ResolveMask{});
    // resolve must detach even if p4 has no mask
    p4 = p4.resolve(p3);
    QCOMPARE(p3.resolveMask(), fullMask);
}


static void compareAllPaletteData(const QPalette &firstPalette, const QPalette &secondPalette)
{
    QCOMPARE(firstPalette, secondPalette);

    // For historical reasons, operator== compares only brushes, but it's not enough for proper
    // comparison after move/copy, because some additional data can also be moved/copied.
    // Let's compare this data here.
    QCOMPARE(firstPalette.resolveMask(), secondPalette.resolveMask());
    QCOMPARE(firstPalette.currentColorGroup(), secondPalette.currentColorGroup());
}

void tst_QPalette::copySemantics()
{
    QPalette src(Qt::red), dst;
    const QPalette control = src; // copy construction
    QVERIFY(src != dst);
    QVERIFY(!src.isCopyOf(dst));
    compareAllPaletteData(src, control);
    QVERIFY(src.isCopyOf(control));
    dst = src; // copy assignment
    compareAllPaletteData(dst, src);
    compareAllPaletteData(dst, control);
    QVERIFY(dst.isCopyOf(src));

    dst = QPalette(Qt::green);
    QVERIFY(dst != src);
    QVERIFY(dst != control);
    compareAllPaletteData(src, control);
    QVERIFY(!dst.isCopyOf(src));
    QVERIFY(src.isCopyOf(control));
}

void tst_QPalette::moveSemantics()
{
    QPalette src(Qt::red), dst;
    const QPalette control = src;
    QVERIFY(src != dst);
    compareAllPaletteData(src, control);
    QVERIFY(!dst.isCopyOf(src));
    QVERIFY(!dst.isCopyOf(control));
    dst = std::move(src); // move assignment
    QVERIFY(!dst.isCopyOf(src)); // isCopyOf() works on moved-from palettes, too
    QVERIFY(dst.isCopyOf(control));
    compareAllPaletteData(dst, control);
    src = control; // check moved-from 'src' can still be assigned to (doesn't crash)
    QVERIFY(src.isCopyOf(dst));
    QVERIFY(src.isCopyOf(control));
    QPalette dst2(std::move(src)); // move construction
    QVERIFY(!src.isCopyOf(dst));
    QVERIFY(!src.isCopyOf(dst2));
    QVERIFY(!src.isCopyOf(control));
    compareAllPaletteData(dst2, control);
    QVERIFY(dst2.isCopyOf(dst));
    QVERIFY(dst2.isCopyOf(control));
    // check moved-from 'src' can still be destroyed (doesn't crash)
}

void tst_QPalette::setBrush()
{
    QPalette p(Qt::red);
    const QPalette q = p;
    QVERIFY(q.isCopyOf(p));

    // Setting a different brush will detach
    p.setBrush(QPalette::Disabled, QPalette::Button, Qt::green);
    QVERIFY(!q.isCopyOf(p));
    QVERIFY(q != p);

    // Check we only changed what we said we would
    for (int i = 0; i < QPalette::NColorGroups; i++)
        for (int j = 0; j < QPalette::NColorRoles; j++) {
            const auto g = QPalette::ColorGroup(i);
            const auto r = QPalette::ColorRole(j);
            const auto b = p.brush(g, r);
            if (g == QPalette::Disabled && r == QPalette::Button)
                QCOMPARE(b, QBrush(Qt::green));
            else
                QCOMPARE(b, q.brush(g, r));
        }

    const QPalette pp = p;
    QVERIFY(pp.isCopyOf(p));
}

void tst_QPalette::isBrushSet()
{
    QPalette p;

    // Set only one color group
    p.setBrush(QPalette::Active, QPalette::Mid, QBrush(Qt::red));
    QVERIFY(p.isBrushSet(QPalette::Active, QPalette::Mid));
    QVERIFY(!p.isBrushSet(QPalette::Inactive, QPalette::Mid));
    QVERIFY(!p.isBrushSet(QPalette::Disabled, QPalette::Mid));

    // Set all color groups
    p.setBrush(QPalette::LinkVisited, QBrush(Qt::green));
    QVERIFY(p.isBrushSet(QPalette::Active, QPalette::LinkVisited));
    QVERIFY(p.isBrushSet(QPalette::Inactive, QPalette::LinkVisited));
    QVERIFY(p.isBrushSet(QPalette::Disabled, QPalette::LinkVisited));

    // Don't set flag when brush doesn't change (and also don't detach - QTBUG-98762)
    QPalette p2;
    QPalette p3;
    QVERIFY(!p2.isBrushSet(QPalette::Active, QPalette::Dark));
    p2.setBrush(QPalette::Active, QPalette::Dark, p2.brush(QPalette::Active, QPalette::Dark));
    QVERIFY(!p3.isBrushSet(QPalette::Active, QPalette::Dark));
    QVERIFY(p2.isBrushSet(QPalette::Active, QPalette::Dark));
}

void tst_QPalette::setAllPossibleBrushes()
{
    QPalette p;

    QCOMPARE(p.resolveMask(), QPalette::ResolveMask(0));

    for (int r = 0; r < QPalette::NColorRoles; ++r) {
        p.setBrush(QPalette::All, QPalette::ColorRole(r), Qt::red);
    }

    for (int r = 0; r < QPalette::NColorRoles; ++r) {
        const QPalette::ColorRole role = static_cast<QPalette::ColorRole>(r);
        for (int g = 0; g < QPalette::NColorGroups; ++g) {
            const QPalette::ColorGroup group = static_cast<QPalette::ColorGroup>(g);
            // NoRole has no resolve bit => isBrushSet returns false
            if (role == QPalette::NoRole)
                QVERIFY(!p.isBrushSet(group, role));
            else
                QVERIFY(p.isBrushSet(group, role));
        }
    }
}

void tst_QPalette::noBrushesSetForDefaultPalette()
{
    QCOMPARE(QPalette().resolveMask(), QPalette::ResolveMask(0));
}

void tst_QPalette::cannotCheckIfInvalidBrushSet()
{
    QPalette p(Qt::red);

    QVERIFY(!p.isBrushSet(QPalette::All, QPalette::LinkVisited));
    QVERIFY(!p.isBrushSet(QPalette::Active, QPalette::NColorRoles));
}

void tst_QPalette::checkIfBrushForCurrentGroupSet()
{
    QPalette p;

    p.setCurrentColorGroup(QPalette::Disabled);
    p.setBrush(QPalette::Current, QPalette::Link, QBrush(Qt::yellow));

    QVERIFY(p.isBrushSet(QPalette::Current, QPalette::Link));
}

void tst_QPalette::cacheKey()
{
    const QPalette defaultPalette;
    // precondition: all palettes are expected to have contrasting text on base
    QVERIFY(defaultPalette.base() != defaultPalette.text());
    const auto defaultCacheKey = defaultPalette.cacheKey();
    const auto defaultSerNo = defaultCacheKey >> 32;
    const auto defaultDetachNo = defaultCacheKey & 0xffffffff;

    QPalette changeTwicePalette(defaultPalette);
    changeTwicePalette.setBrush(QPalette::All, QPalette::ButtonText, Qt::red);
    const auto firstChangeCacheKey = changeTwicePalette.cacheKey();
    QCOMPARE_NE(firstChangeCacheKey, defaultCacheKey);
    changeTwicePalette.setBrush(QPalette::All, QPalette::ButtonText, Qt::green);
    const auto secondChangeCacheKey = changeTwicePalette.cacheKey();
    QCOMPARE_NE(firstChangeCacheKey, secondChangeCacheKey);

    QPalette copyDifferentData(defaultPalette);
    QPalette copyDifferentMask(defaultPalette);
    QPalette copyDifferentMaskAndData(defaultPalette);

    QCOMPARE(defaultPalette.cacheKey(), copyDifferentData.cacheKey());

    // deep detach of both private and data
    copyDifferentData.setBrush(QPalette::Base, defaultPalette.text());
    const auto differentDataKey = copyDifferentData.cacheKey();
    const auto differentDataSerNo = differentDataKey >> 32;
    const auto differentDataDetachNo = differentDataKey & 0xffffffff;
    auto loggerDeepDetach = qScopeGuard([&](){
        qDebug() << "Deep detach serial" << differentDataSerNo;
        qDebug() << "Deep detach detach number" << differentDataDetachNo;
    });

    QCOMPARE_NE(copyDifferentData.cacheKey(), defaultCacheKey);
    QCOMPARE(defaultPalette.cacheKey(), defaultCacheKey);

    // shallow detach, both privates reference the same data
    copyDifferentMask.setResolveMask(0xffffffffffffffff);
    const auto differentMaskKey = copyDifferentMask.cacheKey();
    const auto differentMaskSerNo = differentMaskKey >> 32;
    const auto differentMaskDetachNo = differentMaskKey & 0xffffffff;
    auto loggerShallowDetach = qScopeGuard([&](){
        qDebug() << "Shallow detach serial" << differentMaskSerNo;
        qDebug() << "Shallow detach detach number" << differentMaskDetachNo;
    });

    QCOMPARE(differentMaskSerNo, defaultSerNo);
    QCOMPARE_NE(differentMaskSerNo, defaultDetachNo);
    QCOMPARE_NE(differentMaskKey, defaultCacheKey);
    QCOMPARE_NE(differentMaskKey, differentDataKey);

    // shallow detach, both privates reference the same data
    copyDifferentMaskAndData.setResolveMask(0xeeeeeeeeeeeeeeee);
    const auto modifiedCacheKey = copyDifferentMaskAndData.cacheKey();
    QCOMPARE_NE(modifiedCacheKey, copyDifferentMask.cacheKey());
    QCOMPARE_NE(modifiedCacheKey, defaultCacheKey);
    QCOMPARE_NE(modifiedCacheKey, copyDifferentData.cacheKey());
    QCOMPARE_NE(copyDifferentMask.cacheKey(), defaultCacheKey);

    // full detach - both key elements are different
    copyDifferentMaskAndData.setBrush(QPalette::Base, defaultPalette.text());
    const auto modifiedAllKey = copyDifferentMaskAndData.cacheKey();
    const auto modifiedAllSerNo = modifiedAllKey >> 32;
    const auto modifiedAllDetachNo = modifiedAllKey & 0xffffffff;
    QCOMPARE_NE(modifiedAllSerNo, defaultSerNo);
    QCOMPARE_NE(modifiedAllDetachNo, defaultDetachNo);

    QCOMPARE_NE(modifiedAllKey, copyDifferentMask.cacheKey());
    QCOMPARE_NE(modifiedAllKey, defaultCacheKey);
    QCOMPARE_NE(modifiedAllKey, differentDataKey);
    QCOMPARE_NE(modifiedAllKey, modifiedCacheKey);

    loggerDeepDetach.dismiss();
    loggerShallowDetach.dismiss();
}

void tst_QPalette::dataStream()
{
    const QColor highlight(42, 42, 42);
    const QColor accent(13, 13, 13);
    QPalette palette;
    palette.setBrush(QPalette::Highlight, highlight);
    palette.setBrush(QPalette::Accent, accent);

    // When saved with Qt_6_5 or earlier, Accent defaults to Highlight
    {
        QByteArray b;
        {
            QDataStream stream(&b, QIODevice::WriteOnly);
            stream.setVersion(QDataStream::Qt_6_5);
            stream << palette;
        }
        QPalette test;
        QDataStream stream (&b, QIODevice::ReadOnly);
        stream.setVersion(QDataStream::Qt_6_5);
        stream >> test;
        QCOMPARE(test.accent().color(), highlight);
    }

    // When saved with Qt_6_6 or later, Accent is saved explicitly
    {
        QByteArray b;
        {
            QDataStream stream(&b, QIODevice::WriteOnly);
            stream.setVersion(QDataStream::Qt_6_6);
            stream << palette;
        }
        QPalette test;
        QDataStream stream (&b, QIODevice::ReadOnly);
        stream.setVersion(QDataStream::Qt_6_6);
        stream >> test;
        QCOMPARE(test.accent().color(), accent);
    }
}

QTEST_MAIN(tst_QPalette)
#include "tst_qpalette.moc"
