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


#include <qfont.h>
#include <private/qfont_p.h>
#include <private/qfontengine_p.h>

class tst_QFontCache : public QObject
{
Q_OBJECT

public:
    tst_QFontCache();
    virtual ~tst_QFontCache();

private slots:
    void engineData_data();
    void engineData();
    void engineDataFamilies_data();
    void engineDataFamilies();

    void clear();
};

#ifdef QT_BUILD_INTERNAL
QT_BEGIN_NAMESPACE
// qfontdatabase.cpp
Q_AUTOTEST_EXPORT void qt_setQtEnableTestFont(bool value);
// qfontengine.cpp
Q_AUTOTEST_EXPORT void QFontEngine_startCollectingEngines();
Q_AUTOTEST_EXPORT QList<QFontEngine *> QFontEngine_stopCollectingEngines();
QT_END_NAMESPACE
#endif

tst_QFontCache::tst_QFontCache()
{
}

tst_QFontCache::~tst_QFontCache()
{
}

void tst_QFontCache::engineData_data()
{
    QTest::addColumn<QString>("family");
    QTest::addColumn<QString>("cacheKey");

    QTest::newRow("unquoted-family-name") << QString("Times New Roman") << QString("Times New Roman");
    QTest::newRow("quoted-family-name") << QString("'Times New Roman'") << QString("Times New Roman");
    QTest::newRow("invalid") << QString("invalid") << QString("invalid");
    QTest::newRow("multiple") << QString("invalid, Times New Roman") << QString("invalid,Times New Roman");
    QTest::newRow("multiple spaces") << QString("invalid,  Times New Roman ") << QString("invalid,Times New Roman");
    QTest::newRow("multiple spaces quotes") << QString("'invalid',  Times New Roman ") << QString("invalid,Times New Roman");
    QTest::newRow("multiple2") << QString("invalid, Times New Roman  , foobar, 'baz'") << QString("invalid,Times New Roman,foobar,baz");
    QTest::newRow("invalid spaces") << QString("invalid spaces, Times New Roman ") << QString("invalid spaces,Times New Roman");
    QTest::newRow("invalid spaces quotes") << QString("'invalid spaces', 'Times New Roman' ") << QString("invalid spaces,Times New Roman");
}

void tst_QFontCache::engineData()
{
    QFETCH(QString, family);
    QFETCH(QString, cacheKey);

    QFont f(family);
    f.exactMatch(); // loads engine

    QFontPrivate *d = QFontPrivate::get(f);

    QFontDef req = d->request;
    // copy-pasted from QFontDatabase::load(), to engineer the cache key
    if (req.pixelSize == -1) {
        req.pixelSize = std::floor(((req.pointSize * d->dpi) / 72) * 100 + 0.5) / 100;
        req.pixelSize = qRound(req.pixelSize);
    }
    if (req.pointSize < 0)
        req.pointSize = req.pixelSize*72.0/d->dpi;

    req.family = cacheKey;

    QFontEngineData *engineData = QFontCache::instance()->findEngineData(req);

    QCOMPARE(engineData, QFontPrivate::get(f)->engineData);
}

void tst_QFontCache::engineDataFamilies_data()
{
    QTest::addColumn<QStringList>("families");

    const QStringList multiple = { QLatin1String("invalid"), QLatin1String("Times New Roman") };
    const QStringList multipleQuotes = { QLatin1String("'invalid'"), QLatin1String("Times New Roman") };
    const QStringList multiple2 = { QLatin1String("invalid"), QLatin1String("Times New Roman"),
                                    QLatin1String("foobar"), QLatin1String("'baz'") };

    QTest::newRow("unquoted-family-name") << QStringList(QLatin1String("Times New Roman"));
    QTest::newRow("quoted-family-name") << QStringList(QLatin1String("Times New Roman"));
    QTest::newRow("invalid") << QStringList(QLatin1String("invalid"));
    QTest::newRow("multiple") << multiple;
    QTest::newRow("multiple spaces quotes") << multipleQuotes;
    QTest::newRow("multiple2") << multiple2;
}

void tst_QFontCache::engineDataFamilies()
{
    QFETCH(QStringList, families);

    QFont f;
    f.setFamily(QString()); // Unset the family as taken from the QGuiApplication default
    f.setFamilies(families);
    f.exactMatch(); // loads engine
    QFontPrivate *d = QFontPrivate::get(f);

    QFontDef req = d->request;
    // copy-pasted from QFontDatabase::load(), to engineer the cache key
    if (req.pixelSize == -1) {
        req.pixelSize = std::floor(((req.pointSize * d->dpi) / 72) * 100 + 0.5) / 100;
        req.pixelSize = qRound(req.pixelSize);
    }
    if (req.pointSize < 0)
        req.pointSize = req.pixelSize*72.0/d->dpi;

    req.families = families;

    QFontEngineData *engineData = QFontCache::instance()->findEngineData(req);

    QCOMPARE(engineData, QFontPrivate::get(f)->engineData);
}

void tst_QFontCache::clear()
{
#ifdef QT_BUILD_INTERNAL
    QFontEngine_startCollectingEngines();
#else
    // must not crash, at very least ;)
#endif

    QFontEngine *fontEngine = 0;

#ifdef QT_BUILD_INTERNAL
    {
        // we're never caching the box (and the "test") font engines
        // let's ensure we're not leaking them as well as the cached ones
        qt_setQtEnableTestFont(true);

        QFont f;
        f.setFamily("__Qt__Box__Engine__");
        f.exactMatch(); // loads engine
    }
#endif
    {
        QFontDatabase db;

        QFont f;
        f.setStyleHint(QFont::Serif);
        const QString familyForHint(f.defaultFamily());

        // it should at least return a family that is available
        QVERIFY(db.hasFamily(familyForHint));
        f.exactMatch(); // loads engine

        fontEngine = QFontPrivate::get(f)->engineForScript(QChar::Script_Common);
        QVERIFY(fontEngine);
        QVERIFY(QFontCache::instance()->engineCacheCount.value(fontEngine) > 0); // ensure it is cached

        // acquire the engine to use it somewhere else:
        // (e.g. like the we do in QFontSubset() or like QRawFont does in fromFont())
        fontEngine->ref.ref();

        // cache the engine once again; there is a special case when the engine is cached more than once
        QFontCache::instance()->insertEngine(QFontCache::Key(QFontDef(), 0, 1), fontEngine);
    }

    // use it:
    // e.g. fontEngine->stringToCMap(..);

    // and whilst it is alive, don't hesitate to add/remove the app-local fonts:
    // (QFontDatabase::{add,remove}ApplicationFont() clears the cache)
    QFontCache::instance()->clear();

    // release the acquired engine:
    if (fontEngine) {
        if (!fontEngine->ref.deref())
            delete fontEngine;
        fontEngine = 0;
    }

    // we may even exit the application now:
    QFontCache::instance()->cleanup();

#ifdef QT_BUILD_INTERNAL
    QList<QFontEngine *> leakedEngines = QFontEngine_stopCollectingEngines();
for (int i = 0; i < leakedEngines.size(); ++i) qWarning() << i << leakedEngines.at(i) << leakedEngines.at(i)->ref.loadRelaxed();
    // and we are not leaking!
    QCOMPARE(leakedEngines.size(), 0);
#endif
}

QTEST_MAIN(tst_QFontCache)
#include "tst_qfontcache.moc"
