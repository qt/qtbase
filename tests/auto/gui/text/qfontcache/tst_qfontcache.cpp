/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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
    void clear();
};

QT_BEGIN_NAMESPACE
extern void qt_setQtEnableTestFont(bool value); // qfontdatabase.cpp

#ifdef QT_BUILD_INTERNAL
// qfontengine.cpp
extern void QFontEngine_startCollectingEngines();
extern QList<QFontEngine *> QFontEngine_stopCollectingEngines();
#endif
QT_END_NAMESPACE

tst_QFontCache::tst_QFontCache()
{
}

tst_QFontCache::~tst_QFontCache()
{
}

void tst_QFontCache::clear()
{
#ifdef QT_BUILD_INTERNAL
    QFontEngine_startCollectingEngines();
#else
    // must not crash, at very least ;)
#endif

    QFontEngine *fontEngine = 0;

    {
        // we're never caching the box (and the "test") font engines
        // let's ensure we're not leaking them as well as the cached ones
        qt_setQtEnableTestFont(true);

        QFont f;
        f.setFamily("__Qt__Box__Engine__");
        f.exactMatch(); // loads engine
    }
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
        QFontCache::instance()->insertEngine(QFontCache::Key(QFontDef(), 0, 0), fontEngine);
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
for (int i = 0; i < leakedEngines.size(); ++i) qWarning() << i << leakedEngines.at(i) << leakedEngines.at(i)->ref.load();
    // and we are not leaking!
    QCOMPARE(leakedEngines.size(), 0);
#endif
}

QTEST_MAIN(tst_QFontCache)
#include "tst_qfontcache.moc"
