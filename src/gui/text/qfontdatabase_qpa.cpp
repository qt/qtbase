/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qlibraryinfo.h"
#include <QtCore/qsettings.h>

#include "qfontengine_qpa_p.h"
#include "qplatformdefs.h"

#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformfontdatabase.h>

#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

Q_GUI_EXPORT  void qt_registerFont(const QString &familyName, const QString &stylename,
                                   const QString &foundryname, int weight,
                                   QFont::Style style, int stretch, bool antialiased,
                                   bool scalable, int pixelSize, bool fixedPitch,
                                   const QSupportedWritingSystems &writingSystems, void *handle)
{
    QFontDatabasePrivate *d = privateDb();
//    qDebug() << "Adding font" << familyName << weight << style << pixelSize << antialiased;
    QtFontStyle::Key styleKey;
    styleKey.style = style;
    styleKey.weight = weight;
    styleKey.stretch = stretch;
    QtFontFamily *f = d->family(familyName, true);
    f->fixedPitch = fixedPitch;

    for (int i = 0; i < QFontDatabase::WritingSystemsCount; ++i) {
        if (writingSystems.supported(QFontDatabase::WritingSystem(i)))
            f->writingSystems[i] = QtFontFamily::Supported;
    }

    QtFontFoundry *foundry = f->foundry(foundryname, true);
    QtFontStyle *fontStyle = foundry->style(styleKey, stylename, true);
    fontStyle->smoothScalable = scalable;
    fontStyle->antialiased = antialiased;
    QtFontSize *size = fontStyle->pixelSize(pixelSize ? pixelSize : SMOOTH_SCALABLE, true);
    if (size->handle) {
        QPlatformIntegration *integration = QGuiApplicationPrivate::platformIntegration();
        if (integration)
            integration->fontDatabase()->releaseHandle(size->handle);
    }
    size->handle = handle;
}

Q_GUI_EXPORT void qt_registerAliasToFontFamily(const QString &familyName, const QString &alias)
{
    if (alias.isEmpty())
        return;

    QFontDatabasePrivate *d = privateDb();
    QtFontFamily *f = d->family(familyName, false);
    if (!f)
        return;

    if (f->aliases.contains(alias, Qt::CaseInsensitive))
        return;

    f->aliases.push_back(alias);
}

QString qt_resolveFontFamilyAlias(const QString &alias)
{
    if (!alias.isEmpty()) {
        const QFontDatabasePrivate *d = privateDb();
        for (int i = 0; i < d->count; ++i)
            if (d->families[i]->matchesFamilyName(alias))
                return d->families[i]->name;
    }
    return alias;
}

static QStringList fallbackFamilies(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script)
{
    QStringList retList = QGuiApplicationPrivate::platformIntegration()->fontDatabase()->fallbacksForFamily(family,style,styleHint,script);
    QFontDatabasePrivate *db = privateDb();

    QStringList::iterator i;
    for (i = retList.begin(); i != retList.end(); ++i) {
        bool contains = false;
        for (int j = 0; j < db->count; j++) {
            if (db->families[j]->matchesFamilyName(*i)) {
                contains = true;
                break;
            }
        }
        if (!contains) {
            i = retList.erase(i);
            i--;
        }
    }
    return retList;
}

static void registerFont(QFontDatabasePrivate::ApplicationFont *fnt);

static void initializeDb()
{
    QFontDatabasePrivate *db = privateDb();

    // init by asking for the platformfontdb for the first time or after invalidation
    if (!db->count)
        QGuiApplicationPrivate::platformIntegration()->fontDatabase()->populateFontDatabase();

    if (db->reregisterAppFonts) {
        for (int i = 0; i < db->applicationFonts.count(); i++) {
            if (!db->applicationFonts.at(i).families.isEmpty())
                registerFont(&db->applicationFonts[i]);
        }
        db->reregisterAppFonts = false;
    }
}

static inline void load(const QString & = QString(), int = -1)
{
    // Only initialize the database if it has been cleared or not initialized yet
    if (!privateDb()->count)
        initializeDb();
}

static
QFontEngine *loadSingleEngine(int script,
                              const QFontDef &request,
                              QtFontFoundry *foundry,
                              QtFontStyle *style, QtFontSize *size)
{
    Q_UNUSED(foundry);

    Q_ASSERT(size);
    QPlatformFontDatabase *pfdb = QGuiApplicationPrivate::platformIntegration()->fontDatabase();
    int pixelSize = size->pixelSize;
    if (!pixelSize || (style->smoothScalable && pixelSize == SMOOTH_SCALABLE)
        || pfdb->fontsAlwaysScalable()) {
        pixelSize = request.pixelSize;
    }

    QFontDef def = request;
    def.pixelSize = pixelSize;

    QFontCache::Key key(def,script);
    QFontEngine *engine = QFontCache::instance()->findEngine(key);
    if (!engine) {
        engine = pfdb->fontEngine(def, QChar::Script(script), size->handle);
        if (engine) {
            QFontCache::Key key(def,script);
            QFontCache::instance()->instance()->insertEngine(key,engine);
        }
    }
    return engine;
}

static
QFontEngine *loadEngine(int script, const QFontDef &request,
                        QtFontFamily *family, QtFontFoundry *foundry,
                        QtFontStyle *style, QtFontSize *size)
{

    QFontEngine *engine = loadSingleEngine(script, request, foundry, style, size);
    //make sure that the db has all fallback families
    if (engine && engine->type() != QFontEngine::Multi
        && !(request.styleStrategy & QFont::NoFontMerging) && !engine->symbol ) {

        if (family && !family->askedForFallback) {
            QFont::Style fontStyle = QFont::Style(style->key.style);
            QFont::StyleHint styleHint = QFont::StyleHint(request.styleHint);
            if (styleHint == QFont::AnyStyle && request.fixedPitch)
                styleHint = QFont::TypeWriter;
            family->fallbackFamilies = fallbackFamilies(family->name, fontStyle, styleHint, QChar::Script(script));

            family->askedForFallback = true;
        }

        QStringList fallbacks = privateDb()->fallbackFamilies;
        if (family && !family->fallbackFamilies.isEmpty())
            fallbacks = family->fallbackFamilies;

        QPlatformFontDatabase *pfdb = QGuiApplicationPrivate::platformIntegration()->fontDatabase();
        QFontEngineMulti *pfMultiEngine = pfdb->fontEngineMulti(engine, QChar::Script(script));
        pfMultiEngine->setFallbackFamiliesList(fallbacks);
        engine = pfMultiEngine;

        // Cache Multi font engine as well in case we got the FT single
        // font engine when we are actually looking for a Multi one
        QFontCache::Key key(request, script, 1);
        QFontCache::instance()->instance()->insertEngine(key, engine);
    }

    return engine;
}

static void registerFont(QFontDatabasePrivate::ApplicationFont *fnt)
{
    QFontDatabasePrivate *db = privateDb();

    fnt->families = QGuiApplicationPrivate::platformIntegration()->fontDatabase()->addApplicationFont(fnt->data,fnt->fileName);

    db->reregisterAppFonts = true;
}

bool QFontDatabase::removeApplicationFont(int handle)
{
    QMutexLocker locker(fontDatabaseMutex());

    QFontDatabasePrivate *db = privateDb();
    if (handle < 0 || handle >= db->applicationFonts.count())
        return false;

    db->applicationFonts[handle] = QFontDatabasePrivate::ApplicationFont();

    db->reregisterAppFonts = true;
    db->invalidate();
    return true;
}

bool QFontDatabase::removeAllApplicationFonts()
{
    QMutexLocker locker(fontDatabaseMutex());

    QFontDatabasePrivate *db = privateDb();
    if (db->applicationFonts.isEmpty())
        return false;

    db->applicationFonts.clear();
    db->invalidate();
    return true;
}

bool QFontDatabase::supportsThreadedFontRendering()
{
    return true;
}

/*!
    \internal
*/
QFontEngine *
QFontDatabase::findFont(int script, const QFontPrivate *fp,
                        const QFontDef &request, bool multi)
{
    QMutexLocker locker(fontDatabaseMutex());

    const int force_encoding_id = -1;

    if (!privateDb()->count)
        initializeDb();

    QFontEngine *engine;
    QFontCache::Key key(request, script, multi ? 1 : 0);
    engine = QFontCache::instance()->findEngine(key);
    if (engine) {
        FM_DEBUG("Cache hit level 1");
        return engine;
    }

    QString family_name, foundry_name;

    parseFontName(request.family, foundry_name, family_name);

    if (qt_enable_test_font && request.family == QLatin1String("__Qt__Box__Engine__")) {
        engine =new QTestFontEngine(request.pixelSize);
        engine->fontDef = request;
    }

    QtFontDesc desc;
    match(script, request, family_name, foundry_name, force_encoding_id, &desc);
    if (desc.family != 0 && desc.foundry != 0 && desc.style != 0) {
        engine = loadEngine(script, request, desc.family, desc.foundry, desc.style, desc.size);
    } else {
        FM_DEBUG("  NO MATCH FOUND\n");
    }

    if (engine && engine->type() != QFontEngine::TestFontEngine) {
        initFontDef(desc, request, &engine->fontDef, engine->type() == QFontEngine::Multi);

        if (fp) {
            QFontDef def = request;
            if (def.family.isEmpty()) {
                def.family = fp->request.family;
                def.family = def.family.left(def.family.indexOf(QLatin1Char(',')));
            }
        }
    }

    if (!engine) {
        if (!request.family.isEmpty()) {
            QStringList fallbacks = request.fallBackFamilies
                                  + fallbackFamilies(request.family,
                                                     QFont::Style(request.style),
                                                     QFont::StyleHint(request.styleHint),
                                                     QChar::Script(script));

            for (int i = 0; !engine && i < fallbacks.size(); i++) {
                QFontDef def = request;
                def.family = fallbacks.at(i);
                QFontCache::Key key(def, script, multi ? 1 : 0);
                engine = QFontCache::instance()->findEngine(key);
                if (!engine) {
                    QtFontDesc desc;
                    match(script, def, def.family, QLatin1String(""), 0, &desc);
                    if (desc.family == 0 && desc.foundry == 0 && desc.style == 0) {
                        continue;
                    }
                    engine = loadEngine(script, def, desc.family, desc.foundry, desc.style, desc.size);
                    if (engine) {
                        initFontDef(desc, def, &engine->fontDef, engine->type() == QFontEngine::Multi);
                    }
                }
            }
        }

        if (!engine)
            engine = new QFontEngineBox(request.pixelSize);

        FM_DEBUG("returning box engine");
    }

    if (fp && fp->dpi > 0) {
        engine->fontDef.pointSize = qreal(double((engine->fontDef.pixelSize * 72) / fp->dpi));
    } else {
        engine->fontDef.pointSize = request.pointSize;
    }

    return engine;
}

void QFontDatabase::load(const QFontPrivate *d, int script)
{
    QFontDef req = d->request;

    if (req.pixelSize == -1) {
        req.pixelSize = floor(((req.pointSize * d->dpi) / 72) * 100 + 0.5) / 100;
        req.pixelSize = qRound(req.pixelSize);
    }
    if (req.pointSize < 0)
        req.pointSize = req.pixelSize*72.0/d->dpi;
    if (req.weight == 0)
        req.weight = QFont::Normal;
    if (req.stretch == 0)
        req.stretch = 100;

    // Until we specifically asked not to, try looking for Multi font engine
    // first, the last '1' indicates that we want Multi font engine instead
    // of single ones
    bool multi = !(req.styleStrategy & QFont::NoFontMerging);
    QFontCache::Key key(req, script, multi ? 1 : 0);

    if (!d->engineData)
        getEngineData(d, req);

    // the cached engineData could have already loaded the engine we want
    if (d->engineData->engines[script])
        return;

    QFontEngine *fe = QFontCache::instance()->findEngine(key);

    // list of families to try
    QStringList family_list;

    if (!req.family.isEmpty()) {
        QStringList familiesForRequest = familyList(req);

        // Add primary selection
        family_list << familiesForRequest.takeFirst();

        // Fallbacks requested in font request
        req.fallBackFamilies = familiesForRequest;

        // add the default family
        QString defaultFamily = QGuiApplication::font().family();
        if (! family_list.contains(defaultFamily))
            family_list << defaultFamily;

    }

    // null family means find the first font matching the specified script
    family_list << QString();

    QStringList::ConstIterator it = family_list.constBegin(), end = family_list.constEnd();
    for (; !fe && it != end; ++it) {
        req.family = *it;

        fe = QFontDatabase::findFont(script, d, req, multi);
        if (fe && (fe->type()==QFontEngine::Box) && !req.family.isEmpty()) {
            if (fe->ref.load() == 0)
                delete fe;

            fe = 0;
        }

        // No need to check requested fallback families again
        req.fallBackFamilies.clear();
    }

    if (fe->symbol || (d->request.styleStrategy & QFont::NoFontMerging)) {
        for (int i = 0; i < QChar::ScriptCount; ++i) {
            if (!d->engineData->engines[i]) {
                d->engineData->engines[i] = fe;
                fe->ref.ref();
            }
        }
    } else {
        d->engineData->engines[script] = fe;
        fe->ref.ref();
    }
}

QString QFontDatabase::resolveFontFamilyAlias(const QString &family)
{
    return QGuiApplicationPrivate::platformIntegration()->fontDatabase()->resolveFontFamilyAlias(family);
}

QT_END_NAMESPACE
