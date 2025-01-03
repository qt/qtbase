// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qfontdatabase.h"
#include "qfontdatabase_p.h"
#include "qloggingcategory.h"
#include "qalgorithms.h"
#include "qguiapplication.h"
#include "qvarlengtharray.h" // here or earlier - workaround for VC++6
#include "qthread.h"
#include "qmutex.h"
#include "qfile.h"
#include "qfileinfo.h"
#include "qfontengine_p.h"
#include <qpa/qplatformintegration.h>

#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformfontdatabase.h>
#include <qpa/qplatformtheme.h>

#include <QtCore/qcache.h>
#include <QtCore/qmath.h>

#include <stdlib.h>
#include <algorithm>

#include <qtgui_tracepoints_p.h>

#ifdef Q_OS_WIN
#include <QtGui/private/qwindowsfontdatabasebase_p.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcFontDb, "qt.text.font.db")
Q_LOGGING_CATEGORY(lcFontMatch, "qt.text.font.match")

#define SMOOTH_SCALABLE 0xffff

#if defined(QT_BUILD_INTERNAL)
bool qt_enable_test_font = false;

Q_AUTOTEST_EXPORT void qt_setQtEnableTestFont(bool value)
{
    qt_enable_test_font = value;
}
#endif

Q_TRACE_POINT(qtgui, QFontDatabase_loadEngine, const QString &families, int pointSize);
Q_TRACE_POINT(qtgui, QFontDatabasePrivate_addAppFont, const QString &fileName);
Q_TRACE_POINT(qtgui, QFontDatabase_addApplicationFont, const QString &fileName);
Q_TRACE_POINT(qtgui, QFontDatabase_load, const QString &family, int pointSize);

static int getFontWeight(const QString &weightString)
{
    QString s = weightString.toLower();

    // Order here is important. We want to match the common cases first, but we
    // must also take care to acknowledge the cost of our tests.
    //
    // As a result, we test in two orders; the order of commonness, and the
    // order of "expense".
    //
    // A simple string test is the cheapest, so let's do that first.
    // Test in decreasing order of commonness
    if (s == "normal"_L1 || s == "regular"_L1)
        return QFont::Normal;
    if (s == "bold"_L1)
        return QFont::Bold;
    if (s == "semibold"_L1 || s == "semi bold"_L1 || s == "demibold"_L1 || s == "demi bold"_L1)
        return QFont::DemiBold;
    if (s == "medium"_L1)
        return QFont::Medium;
    if (s == "black"_L1)
        return QFont::Black;
    if (s == "light"_L1)
        return QFont::Light;
    if (s == "thin"_L1)
        return QFont::Thin;
    const QStringView s2 = QStringView{s}.mid(2);
    if (s.startsWith("ex"_L1) || s.startsWith("ul"_L1)) {
            if (s2 == "tralight"_L1 || s == "tra light"_L1)
                return QFont::ExtraLight;
            if (s2 == "trabold"_L1 || s2 == "tra bold"_L1)
                return QFont::ExtraBold;
    }

    // Next up, let's see if contains() matches: slightly more expensive, but
    // still fast enough.
    if (s.contains("bold"_L1)) {
        if (s.contains("demi"_L1))
            return QFont::DemiBold;
        return QFont::Bold;
    }
    if (s.contains("thin"_L1))
        return QFont::Thin;
    if (s.contains("light"_L1))
        return QFont::Light;
    if (s.contains("black"_L1))
        return QFont::Black;

    // Now, we perform string translations & comparisons with those.
    // These are (very) slow compared to simple string ops, so we do these last.
    // As using translated values for such things is not very common, this should
    // not be too bad.
    if (s.compare(QCoreApplication::translate("QFontDatabase", "Normal", "The Normal or Regular font weight"), Qt::CaseInsensitive) == 0)
        return QFont::Normal;
    const QString translatedBold = QCoreApplication::translate("QFontDatabase", "Bold").toLower();
    if (s == translatedBold)
        return QFont::Bold;
    if (s.compare(QCoreApplication::translate("QFontDatabase", "Demi Bold"), Qt::CaseInsensitive) == 0)
        return QFont::DemiBold;
    if (s.compare(QCoreApplication::translate("QFontDatabase", "Medium", "The Medium font weight"), Qt::CaseInsensitive) == 0)
        return QFont::Medium;
    if (s.compare(QCoreApplication::translate("QFontDatabase", "Black"), Qt::CaseInsensitive) == 0)
        return QFont::Black;
    const QString translatedLight = QCoreApplication::translate("QFontDatabase", "Light").toLower();
    if (s == translatedLight)
        return QFont::Light;
    if (s.compare(QCoreApplication::translate("QFontDatabase", "Thin"), Qt::CaseInsensitive) == 0)
        return QFont::Thin;
    if (s.compare(QCoreApplication::translate("QFontDatabase", "Extra Light"), Qt::CaseInsensitive) == 0)
        return QFont::ExtraLight;
    if (s.compare(QCoreApplication::translate("QFontDatabase", "Extra Bold"), Qt::CaseInsensitive) == 0)
        return QFont::ExtraBold;

    // And now the contains() checks for the translated strings.
    //: The word for "Extra" as in "Extra Bold, Extra Thin" used as a pattern for string searches
    const QString translatedExtra = QCoreApplication::translate("QFontDatabase", "Extra").toLower();
    if (s.contains(translatedBold)) {
        //: The word for "Demi" as in "Demi Bold" used as a pattern for string searches
        QString translatedDemi = QCoreApplication::translate("QFontDatabase", "Demi").toLower();
        if (s .contains(translatedDemi))
            return QFont::DemiBold;
        if (s.contains(translatedExtra))
            return QFont::ExtraBold;
        return QFont::Bold;
    }

    if (s.contains(translatedLight)) {
        if (s.contains(translatedExtra))
            return QFont::ExtraLight;
        return QFont::Light;
    }
    return QFont::Normal;
}


QtFontStyle::Key::Key(const QString &styleString)
    : style(QFont::StyleNormal), weight(QFont::Normal), stretch(0)
{
    weight = getFontWeight(styleString);

    if (!styleString.isEmpty()) {
        // First the straightforward no-translation checks, these are fast.
        if (styleString.contains("Italic"_L1))
            style = QFont::StyleItalic;
        else if (styleString.contains("Oblique"_L1))
            style = QFont::StyleOblique;

        // Then the translation checks. These aren't as fast.
        else if (styleString.contains(QCoreApplication::translate("QFontDatabase", "Italic")))
            style = QFont::StyleItalic;
        else if (styleString.contains(QCoreApplication::translate("QFontDatabase", "Oblique")))
            style = QFont::StyleOblique;
    }
}

QtFontSize *QtFontStyle::pixelSize(unsigned short size, bool add)
{
    for (int i = 0; i < count; i++) {
        if (pixelSizes[i].pixelSize == size)
            return pixelSizes + i;
    }
    if (!add)
        return nullptr;

    if (!pixelSizes) {
        // Most style have only one font size, we avoid waisting memory
        QtFontSize *newPixelSizes = (QtFontSize *)malloc(sizeof(QtFontSize));
        Q_CHECK_PTR(newPixelSizes);
        pixelSizes = newPixelSizes;
    } else if (!(count % 8) || count == 1) {
        QtFontSize *newPixelSizes = (QtFontSize *)
                     realloc(pixelSizes,
                              (((count+8) >> 3) << 3) * sizeof(QtFontSize));
        Q_CHECK_PTR(newPixelSizes);
        pixelSizes = newPixelSizes;
    }
    pixelSizes[count].pixelSize = size;
    pixelSizes[count].handle = nullptr;
    return pixelSizes + (count++);
}

QtFontStyle *QtFontFoundry::style(const QtFontStyle::Key &key, const QString &styleName, bool create)
{
    int pos = 0;
    for (; pos < count; pos++) {
        bool hasStyleName = !styleName.isEmpty(); // search styleName first if available
        if (hasStyleName && !styles[pos]->styleName.isEmpty()) {
            if (styles[pos]->styleName == styleName)
                return styles[pos];
        } else {
            if (styles[pos]->key == key)
                return styles[pos];
        }
    }
    if (!create)
        return nullptr;

//     qDebug("adding key (weight=%d, style=%d, oblique=%d stretch=%d) at %d", key.weight, key.style, key.oblique, key.stretch, pos);
    if (!(count % 8)) {
        QtFontStyle **newStyles = (QtFontStyle **)
                 realloc(styles, (((count+8) >> 3) << 3) * sizeof(QtFontStyle *));
        Q_CHECK_PTR(newStyles);
        styles = newStyles;
    }

    QtFontStyle *style = new QtFontStyle(key);
    style->styleName = styleName;
    styles[pos] = style;
    count++;
    return styles[pos];
}

QtFontFoundry *QtFontFamily::foundry(const QString &f, bool create)
{
    if (f.isNull() && count == 1)
        return foundries[0];

    for (int i = 0; i < count; i++) {
        if (foundries[i]->name.compare(f, Qt::CaseInsensitive) == 0)
            return foundries[i];
    }
    if (!create)
        return nullptr;

    if (!(count % 8)) {
        QtFontFoundry **newFoundries = (QtFontFoundry **)
                    realloc(foundries,
                             (((count+8) >> 3) << 3) * sizeof(QtFontFoundry *));
        Q_CHECK_PTR(newFoundries);
        foundries = newFoundries;
    }

    foundries[count] = new QtFontFoundry(f);
    return foundries[count++];
}

static inline bool equalsCaseInsensitive(const QString &a, const QString &b)
{
    return a.size() == b.size() && a.compare(b, Qt::CaseInsensitive) == 0;
}

bool QtFontFamily::matchesFamilyName(const QString &familyName) const
{
    return equalsCaseInsensitive(name, familyName) || aliases.contains(familyName, Qt::CaseInsensitive);
}

bool QtFontFamily::ensurePopulated()
{
    if (populated)
        return true;

    QGuiApplicationPrivate::platformIntegration()->fontDatabase()->populateFamily(name);
    return populated;
}

void QFontDatabasePrivate::clearFamilies()
{
    while (count--)
        delete families[count];
    ::free(families);
    families = nullptr;
    count = 0;

    for (auto &font : applicationFonts)
        font.properties.clear(); // Unpopulate

    populated = false;
    // don't clear the memory fonts!
}

void QFontDatabasePrivate::invalidate()
{
    qCDebug(lcFontDb) << "Invalidating font database";

    QFontCache::instance()->clear();

    fallbacksCache.clear();
    clearFamilies();
    QGuiApplicationPrivate::platformIntegration()->fontDatabase()->invalidate();
    emit qGuiApp->fontDatabaseChanged();
}

QtFontFamily *QFontDatabasePrivate::family(const QString &f, FamilyRequestFlags flags)
{
    QtFontFamily *fam = nullptr;

    int low = 0;
    int high = count;
    int pos = count / 2;
    int res = 1;
    if (count) {
        while ((res = families[pos]->name.compare(f, Qt::CaseInsensitive)) && pos != low) {
            if (res > 0)
                high = pos;
            else
                low = pos;
            pos = (high + low) / 2;
        }
        if (!res)
            fam = families[pos];
    }

    if (!fam && (flags & EnsureCreated)) {
        if (res < 0)
            pos++;

        // qDebug() << "adding family " << f.toLatin1() << " at " << pos << " total=" << count;
        if (!(count % 8)) {
            QtFontFamily **newFamilies = (QtFontFamily **)
                       realloc(families,
                                (((count+8) >> 3) << 3) * sizeof(QtFontFamily *));
            Q_CHECK_PTR(newFamilies);
            families = newFamilies;
        }

        QtFontFamily *family = new QtFontFamily(f);
        memmove(families + pos + 1, families + pos, (count-pos)*sizeof(QtFontFamily *));
        families[pos] = family;
        count++;

        fam = families[pos];
    }

    if (fam && (flags & EnsurePopulated)) {
        if (!fam->ensurePopulated())
            return nullptr;
    }

    return fam;
}



static const int scriptForWritingSystem[] = {
    QChar::Script_Common, // Any
    QChar::Script_Latin, // Latin
    QChar::Script_Greek, // Greek
    QChar::Script_Cyrillic, // Cyrillic
    QChar::Script_Armenian, // Armenian
    QChar::Script_Hebrew, // Hebrew
    QChar::Script_Arabic, // Arabic
    QChar::Script_Syriac, // Syriac
    QChar::Script_Thaana, // Thaana
    QChar::Script_Devanagari, // Devanagari
    QChar::Script_Bengali, // Bengali
    QChar::Script_Gurmukhi, // Gurmukhi
    QChar::Script_Gujarati, // Gujarati
    QChar::Script_Oriya, // Oriya
    QChar::Script_Tamil, // Tamil
    QChar::Script_Telugu, // Telugu
    QChar::Script_Kannada, // Kannada
    QChar::Script_Malayalam, // Malayalam
    QChar::Script_Sinhala, // Sinhala
    QChar::Script_Thai, // Thai
    QChar::Script_Lao, // Lao
    QChar::Script_Tibetan, // Tibetan
    QChar::Script_Myanmar, // Myanmar
    QChar::Script_Georgian, // Georgian
    QChar::Script_Khmer, // Khmer
    QChar::Script_Han, // SimplifiedChinese
    QChar::Script_Han, // TraditionalChinese
    QChar::Script_Han, // Japanese
    QChar::Script_Hangul, // Korean
    QChar::Script_Latin, // Vietnamese
    QChar::Script_Common, // Symbol
    QChar::Script_Ogham,  // Ogham
    QChar::Script_Runic, // Runic
    QChar::Script_Nko // Nko
};

static_assert(sizeof(scriptForWritingSystem) / sizeof(scriptForWritingSystem[0]) == QFontDatabase::WritingSystemsCount);

Q_GUI_EXPORT int qt_script_for_writing_system(QFontDatabase::WritingSystem writingSystem)
{
    return scriptForWritingSystem[writingSystem];
}


/*!
    \internal

    Tests if the given family \a family supports writing system \a writingSystem,
    including the special case for Han script mapping to several subsequent writing systems
*/
static bool familySupportsWritingSystem(QtFontFamily *family, size_t writingSystem)
{
    Q_ASSERT(family != nullptr);
    Q_ASSERT(writingSystem != QFontDatabase::Any && writingSystem < QFontDatabase::WritingSystemsCount);

    size_t ws = writingSystem;
    do {
        if ((family->writingSystems[ws] & QtFontFamily::Supported) != 0)
            return true;
    } while (writingSystem >= QFontDatabase::SimplifiedChinese && writingSystem <= QFontDatabase::Japanese && ++ws <= QFontDatabase::Japanese);

    return false;
}

Q_GUI_EXPORT QFontDatabase::WritingSystem qt_writing_system_for_script(int script)
{
    return QFontDatabase::WritingSystem(std::find(scriptForWritingSystem,
                                                  scriptForWritingSystem + QFontDatabase::WritingSystemsCount,
                                                  script) - scriptForWritingSystem);
}

/*!
  \internal

  This makes sense of the font family name:

  if the family name contains a '[' and a ']', then we take the text
  between the square brackets as the foundry, and the text before the
  square brackets as the family (ie. "Arial [Monotype]")
*/
static void parseFontName(const QString &name, QString &foundry, QString &family)
{
    int i = name.indexOf(u'[');
    int li = name.lastIndexOf(u']');
    if (i >= 0 && li >= 0 && i < li) {
        foundry = name.mid(i + 1, li - i - 1);
        if (i > 0 && name[i - 1] == u' ')
            i--;
        family = name.left(i);
    } else {
        foundry.clear();
        family = name;
    }

    // capitalize the family/foundry names
    bool space = true;
    QChar *s = family.data();
    int len = family.size();
    while(len--) {
        if (space) *s = s->toUpper();
        space = s->isSpace();
        ++s;
    }

    space = true;
    s = foundry.data();
    len = foundry.size();
    while(len--) {
        if (space) *s = s->toUpper();
        space = s->isSpace();
        ++s;
    }
}


struct QtFontDesc
{
    inline QtFontDesc() : family(nullptr), foundry(nullptr), style(nullptr), size(nullptr) {}
    QtFontFamily *family;
    QtFontFoundry *foundry;
    QtFontStyle *style;
    QtFontSize *size;
};

static void initFontDef(const QtFontDesc &desc, const QFontDef &request, QFontDef *fontDef, bool multi)
{
    QString family;
    family = desc.family->name;
    if (! desc.foundry->name.isEmpty() && desc.family->count > 1)
        family += " ["_L1 + desc.foundry->name + u']';
    fontDef->families = QStringList(family);

    if (desc.style->smoothScalable
        || QGuiApplicationPrivate::platformIntegration()->fontDatabase()->fontsAlwaysScalable()
        || (desc.style->bitmapScalable && (request.styleStrategy & QFont::PreferMatch))) {
        fontDef->pixelSize = request.pixelSize;
    } else {
        fontDef->pixelSize = desc.size->pixelSize;
    }
    fontDef->pointSize     = request.pointSize;

    fontDef->styleHint     = request.styleHint;
    fontDef->styleStrategy = request.styleStrategy;

    if (!multi)
        fontDef->weight    = desc.style->key.weight;
    if (!multi)
        fontDef->style     = desc.style->key.style;
    fontDef->fixedPitch    = desc.family->fixedPitch;
    fontDef->ignorePitch   = false;
}

static QStringList familyList(const QFontDef &req)
{
    // list of families to try
    QStringList family_list;

    family_list << req.families;
    // append the substitute list for each family in family_list
    for (int i = 0, size = family_list.size(); i < size; ++i)
        family_list += QFont::substitutes(family_list.at(i));

    return family_list;
}

Q_GLOBAL_STATIC(QRecursiveMutex, fontDatabaseMutex)

// used in qguiapplication.cpp
void qt_cleanupFontDatabase()
{
    auto *db = QFontDatabasePrivate::instance();
    db->fallbacksCache.clear();
    db->clearFamilies();
}

// used in qfont.cpp
QRecursiveMutex *qt_fontdatabase_mutex()
{
    return fontDatabaseMutex();
}

QFontDatabasePrivate *QFontDatabasePrivate::instance()
{
    static QFontDatabasePrivate instance;
    return &instance;
}

void qt_registerFont(const QString &familyName, const QString &stylename,
                     const QString &foundryname, int weight,
                     QFont::Style style, int stretch, bool antialiased,
                     bool scalable, int pixelSize, bool fixedPitch,
                     const QSupportedWritingSystems &writingSystems, void *handle)
{
    auto *d = QFontDatabasePrivate::instance();
    qCDebug(lcFontDb) << "Adding font: familyName" << familyName << "stylename" << stylename << "weight" << weight
        << "style" << style << "pixelSize" << pixelSize << "antialiased" << antialiased << "fixed" << fixedPitch;
    QtFontStyle::Key styleKey;
    styleKey.style = style;
    styleKey.weight = weight;
    styleKey.stretch = stretch;
    QtFontFamily *f = d->family(familyName, QFontDatabasePrivate::EnsureCreated);
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
    f->populated = true;
}

void qt_registerFontFamily(const QString &familyName)
{
    qCDebug(lcFontDb) << "Registering family" << familyName;

    // Create uninitialized/unpopulated family
    QFontDatabasePrivate::instance()->family(familyName, QFontDatabasePrivate::EnsureCreated);
}

void qt_registerAliasToFontFamily(const QString &familyName, const QString &alias)
{
    if (alias.isEmpty())
        return;

    qCDebug(lcFontDb) << "Registering alias" << alias << "to family" << familyName;

    auto *d = QFontDatabasePrivate::instance();
    QtFontFamily *f = d->family(familyName, QFontDatabasePrivate::RequestFamily);
    if (!f)
        return;

    if (f->aliases.contains(alias, Qt::CaseInsensitive))
        return;

    f->aliases.push_back(alias);
}

QString qt_resolveFontFamilyAlias(const QString &alias)
{
    if (!alias.isEmpty()) {
        const auto *d = QFontDatabasePrivate::instance();
        for (int i = 0; i < d->count; ++i)
            if (d->families[i]->matchesFamilyName(alias))
                return d->families[i]->name;
    }
    return alias;
}

bool qt_isFontFamilyPopulated(const QString &familyName)
{
    auto *d = QFontDatabasePrivate::instance();
    QtFontFamily *f = d->family(familyName, QFontDatabasePrivate::RequestFamily);
    return f != nullptr && f->populated;
}

/*!
    Returns a list of alternative fonts for the specified \a family and
    \a style and \a script using the \a styleHint given.

    Default implementation returns a list of fonts for which \a style and \a script support
    has been reported during the font database population.
*/
QStringList QPlatformFontDatabase::fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script) const
{
    Q_UNUSED(family);
    Q_UNUSED(styleHint);

    QStringList preferredFallbacks;
    QStringList otherFallbacks;

    auto writingSystem = qt_writing_system_for_script(script);
    if (writingSystem >= QFontDatabase::WritingSystemsCount)
        writingSystem = QFontDatabase::Any;

    auto *db = QFontDatabasePrivate::instance();
    for (int i = 0; i < db->count; ++i) {
        QtFontFamily *f = db->families[i];

        f->ensurePopulated();

        if (writingSystem != QFontDatabase::Any && !familySupportsWritingSystem(f, writingSystem))
            continue;

        for (int j = 0; j < f->count; ++j) {
            QtFontFoundry *foundry = f->foundries[j];

            for (int k = 0; k < foundry->count; ++k) {
                QString name = foundry->name.isEmpty()
                        ? f->name
                        : f->name + " ["_L1 + foundry->name + u']';
                if (style == foundry->styles[k]->key.style)
                    preferredFallbacks.append(name);
                else
                    otherFallbacks.append(name);
            }
        }
    }

    return preferredFallbacks + otherFallbacks;
}

static QStringList fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script)
{
    QMutexLocker locker(fontDatabaseMutex());
    auto *db = QFontDatabasePrivate::ensureFontDatabase();

    const QtFontFallbacksCacheKey cacheKey = { family, style, styleHint, script };

    if (const QStringList *fallbacks = db->fallbacksCache.object(cacheKey))
        return *fallbacks;

    // make sure that the db has all fallback families
    QStringList retList = QGuiApplicationPrivate::platformIntegration()->fontDatabase()->fallbacksForFamily(family,style,styleHint,script);

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
            --i;
        }
    }

    db->fallbacksCache.insert(cacheKey, new QStringList(retList));

    return retList;
}

QStringList qt_fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script)
{
    QMutexLocker locker(fontDatabaseMutex());
    return fallbacksForFamily(family, style, styleHint, script);
}

QFontEngine *QFontDatabasePrivate::loadSingleEngine(int script,
                              const QFontDef &request,
                              QtFontFamily *family, QtFontFoundry *foundry,
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

    QFontCache *fontCache = QFontCache::instance();

    QFontCache::Key key(def,script);
    QFontEngine *engine = fontCache->findEngine(key);
    if (!engine) {
        const bool cacheForCommonScript = script != QChar::Script_Common
                && (family->writingSystems[QFontDatabase::Latin] & QtFontFamily::Supported) != 0;

        if (Q_LIKELY(cacheForCommonScript)) {
            // fast path: check if engine was loaded for another script
            key.script = QChar::Script_Common;
            engine = fontCache->findEngine(key);
            key.script = script;
            if (engine) {
                // Also check for OpenType tables when using complex scripts
                if (Q_UNLIKELY(!engine->supportsScript(QChar::Script(script)))) {
                    qWarning("  OpenType support missing for \"%s\", script %d",
                             qPrintable(def.families.first()), script);
                    return nullptr;
                }

                engine->isSmoothlyScalable = style->smoothScalable;
                fontCache->insertEngine(key, engine);
                return engine;
            }
        }

        // To avoid synthesized stretch we need a matching stretch to be 100 after this point.
        // If stretch didn't match exactly we need to calculate the new stretch factor.
        // This only done if not matched by styleName.
        if (style->key.stretch != 0 && request.stretch != 0
            && (request.styleName.isEmpty() || request.styleName != style->styleName)) {
            def.stretch = (request.stretch * 100 + style->key.stretch / 2) / style->key.stretch;
        } else if (request.stretch == QFont::AnyStretch) {
            def.stretch = 100;
        }

        engine = pfdb->fontEngine(def, size->handle);
        if (engine) {
            // Also check for OpenType tables when using complex scripts
            if (!engine->supportsScript(QChar::Script(script))) {
                qWarning("  OpenType support missing for \"%s\", script %d",
                         +qPrintable(def.families.first()), script);
                if (engine->ref.loadRelaxed() == 0)
                    delete engine;
                return nullptr;
            }

            engine->isSmoothlyScalable = style->smoothScalable;
            fontCache->insertEngine(key, engine);

            if (Q_LIKELY(cacheForCommonScript && !engine->symbol)) {
                // cache engine for Common script as well
                key.script = QChar::Script_Common;
                if (!fontCache->findEngine(key))
                    fontCache->insertEngine(key, engine);
            }
        }
    }
    return engine;
}

QFontEngine *QFontDatabasePrivate::loadEngine(int script, const QFontDef &request,
                        QtFontFamily *family, QtFontFoundry *foundry,
                        QtFontStyle *style, QtFontSize *size)
{
    QFontEngine *engine = loadSingleEngine(script, request, family, foundry, style, size);

    if (engine && !(request.styleStrategy & QFont::NoFontMerging) && !engine->symbol) {
        Q_TRACE(QFontDatabase_loadEngine, request.families.join(QLatin1Char(';')), request.pointSize);

        QPlatformFontDatabase *pfdb = QGuiApplicationPrivate::platformIntegration()->fontDatabase();
        QFontEngineMulti *pfMultiEngine = pfdb->fontEngineMulti(engine, QChar::Script(script));
        if (!request.fallBackFamilies.isEmpty()) {
            QStringList fallbacks = request.fallBackFamilies;

            QFont::StyleHint styleHint = QFont::StyleHint(request.styleHint);
            if (styleHint == QFont::AnyStyle && request.fixedPitch)
                styleHint = QFont::TypeWriter;

            fallbacks += fallbacksForFamily(family->name, QFont::Style(style->key.style), styleHint, QChar::Script(script));

            pfMultiEngine->setFallbackFamiliesList(fallbacks);
        }
        engine = pfMultiEngine;

        // Cache Multi font engine as well in case we got the single
        // font engine when we are actually looking for a Multi one
        QFontCache::Key key(request, script, 1);
        QFontCache::instance()->insertEngine(key, engine);
    }

    return engine;
}

QtFontStyle::~QtFontStyle()
{
   while (count) {
       // bitfield count-- in while condition does not work correctly in mwccsym2
       count--;
       QPlatformIntegration *integration = QGuiApplicationPrivate::platformIntegration();
       if (integration)
           integration->fontDatabase()->releaseHandle(pixelSizes[count].handle);
   }

   free(pixelSizes);
}

static QtFontStyle *bestStyle(QtFontFoundry *foundry, const QtFontStyle::Key &styleKey,
                              const QString &styleName = QString())
{
    int best = 0;
    int dist = 0xffff;

    for ( int i = 0; i < foundry->count; i++ ) {
        QtFontStyle *style = foundry->styles[i];

        if (!styleName.isEmpty() && styleName == style->styleName) {
            dist = 0;
            best = i;
            break;
        }

        int d = qAbs( (int(styleKey.weight) - int(style->key.weight)) / 10 );

        if ( styleKey.stretch != 0 && style->key.stretch != 0 ) {
            d += qAbs( styleKey.stretch - style->key.stretch );
        }

        if (styleKey.style != style->key.style) {
            if (styleKey.style != QFont::StyleNormal && style->key.style != QFont::StyleNormal)
                // one is italic, the other oblique
                d += 0x0001;
            else
                d += 0x1000;
        }

        if ( d < dist ) {
            best = i;
            dist = d;
        }
    }

    qCDebug(lcFontMatch,  "          best style has distance 0x%x", dist );
    return foundry->styles[best];
}


unsigned int QFontDatabasePrivate::bestFoundry(int script, unsigned int score, int styleStrategy,
                         const QtFontFamily *family, const QString &foundry_name,
                         QtFontStyle::Key styleKey, int pixelSize, char pitch,
                         QtFontDesc *desc, const QString &styleName)
{
    Q_UNUSED(script);
    Q_UNUSED(pitch);

    desc->foundry = nullptr;
    desc->style = nullptr;
    desc->size = nullptr;


    qCDebug(lcFontMatch, "  REMARK: looking for best foundry for family '%s' [%d]", family->name.toLatin1().constData(), family->count);

    for (int x = 0; x < family->count; ++x) {
        QtFontFoundry *foundry = family->foundries[x];
        if (!foundry_name.isEmpty() && foundry->name.compare(foundry_name, Qt::CaseInsensitive) != 0)
            continue;

        qCDebug(lcFontMatch, "          looking for matching style in foundry '%s' %d",
                 foundry->name.isEmpty() ? "-- none --" : foundry->name.toLatin1().constData(), foundry->count);

        QtFontStyle *style = bestStyle(foundry, styleKey, styleName);

        if (!style->smoothScalable && (styleStrategy & QFont::ForceOutline)) {
            qCDebug(lcFontMatch, "            ForceOutline set, but not smoothly scalable");
            continue;
        }

        int px = -1;
        QtFontSize *size = nullptr;

        // 1. see if we have an exact matching size
        if (!(styleStrategy & QFont::ForceOutline)) {
            size = style->pixelSize(pixelSize);
            if (size) {
                qCDebug(lcFontMatch, "          found exact size match (%d pixels)", size->pixelSize);
                px = size->pixelSize;
            }
        }

        // 2. see if we have a smoothly scalable font
        if (!size && style->smoothScalable && ! (styleStrategy & QFont::PreferBitmap)) {
            size = style->pixelSize(SMOOTH_SCALABLE);
            if (size) {
                qCDebug(lcFontMatch, "          found smoothly scalable font (%d pixels)", pixelSize);
                px = pixelSize;
            }
        }

        // 3. see if we have a bitmap scalable font
        if (!size && style->bitmapScalable && (styleStrategy & QFont::PreferMatch)) {
            size = style->pixelSize(0);
            if (size) {
                qCDebug(lcFontMatch, "          found bitmap scalable font (%d pixels)", pixelSize);
                px = pixelSize;
            }
        }


        // 4. find closest size match
        if (! size) {
            unsigned int distance = ~0u;
            for (int x = 0; x < style->count; ++x) {

                unsigned int d;
                if (style->pixelSizes[x].pixelSize < pixelSize) {
                    // penalize sizes that are smaller than the
                    // requested size, due to truncation from floating
                    // point to integer conversions
                    d = pixelSize - style->pixelSizes[x].pixelSize + 1;
                } else {
                    d = style->pixelSizes[x].pixelSize - pixelSize;
                }

                if (d < distance) {
                    distance = d;
                    size = style->pixelSizes + x;
                    qCDebug(lcFontMatch, "          best size so far: %3d (%d)", size->pixelSize, pixelSize);
                }
            }

            if (!size) {
                qCDebug(lcFontMatch, "          no size supports the script we want");
                continue;
            }

            if (style->bitmapScalable && ! (styleStrategy & QFont::PreferQuality) &&
                (distance * 10 / pixelSize) >= 2) {
                // the closest size is not close enough, go ahead and
                // use a bitmap scaled font
                size = style->pixelSize(0);
                px = pixelSize;
            } else {
                px = size->pixelSize;
            }
        }


        unsigned int this_score = 0x0000;
        enum {
            PitchMismatch       = 0x4000,
            StyleMismatch       = 0x2000,
            BitmapScaledPenalty = 0x1000
        };
        if (pitch != '*') {
            if ((pitch == 'm' && !family->fixedPitch)
                || (pitch == 'p' && family->fixedPitch))
                this_score += PitchMismatch;
        }
        if (styleKey != style->key)
            this_score += StyleMismatch;
        if (!style->smoothScalable && px != size->pixelSize) // bitmap scaled
            this_score += BitmapScaledPenalty;
        if (px != pixelSize) // close, but not exact, size match
            this_score += qAbs(px - pixelSize);

        if (this_score < score) {
            qCDebug(lcFontMatch, "          found a match: score %x best score so far %x",
                     this_score, score);

            score = this_score;
            desc->foundry = foundry;
            desc->style = style;
            desc->size = size;
        } else {
            qCDebug(lcFontMatch, "          score %x no better than best %x", this_score, score);
        }
    }

    return score;
}

static bool matchFamilyName(const QString &familyName, QtFontFamily *f)
{
    if (familyName.isEmpty())
        return true;
    return f->matchesFamilyName(familyName);
}

/*!
    \internal

    Tries to find the best match for a given request and family/foundry
*/
int QFontDatabasePrivate::match(int script, const QFontDef &request, const QString &family_name,
                     const QString &foundry_name, QtFontDesc *desc, const QList<int> &blacklistedFamilies,
                     unsigned int *resultingScore)
{
    int result = -1;

    QtFontStyle::Key styleKey;
    styleKey.style = request.style;
    styleKey.weight = request.weight;
    // Prefer a stretch closest to 100.
    styleKey.stretch = request.stretch ? request.stretch : 100;
    char pitch = request.ignorePitch ? '*' : request.fixedPitch ? 'm' : 'p';


    qCDebug(lcFontMatch, "QFontDatabasePrivate::match\n"
             "  request:\n"
             "    family: %s [%s], script: %d\n"
             "    styleName: %s\n"
             "    weight: %d, style: %d\n"
             "    stretch: %d\n"
             "    pixelSize: %g\n"
             "    pitch: %c",
             family_name.isEmpty() ? "-- first in script --" : family_name.toLatin1().constData(),
             foundry_name.isEmpty() ? "-- any --" : foundry_name.toLatin1().constData(), script,
             request.styleName.isEmpty() ? "-- any --" : request.styleName.toLatin1().constData(),
             request.weight, request.style, request.stretch, request.pixelSize, pitch);

    desc->family = nullptr;
    desc->foundry = nullptr;
    desc->style = nullptr;
    desc->size = nullptr;

    unsigned int score = ~0u;

    QMutexLocker locker(fontDatabaseMutex());
    QFontDatabasePrivate::ensureFontDatabase();

    auto writingSystem = qt_writing_system_for_script(script);
    if (writingSystem >= QFontDatabase::WritingSystemsCount)
        writingSystem = QFontDatabase::Any;

    auto *db = QFontDatabasePrivate::instance();
    for (int x = 0; x < db->count; ++x) {
        if (blacklistedFamilies.contains(x))
            continue;
        QtFontDesc test;
        test.family = db->families[x];

        if (!matchFamilyName(family_name, test.family))
            continue;
        if (!test.family->ensurePopulated())
            continue;

        // Check if family is supported in the script we want
        if (writingSystem != QFontDatabase::Any && !familySupportsWritingSystem(test.family, writingSystem))
            continue;

        // as we know the script is supported, we can be sure
        // to find a matching font here.
        unsigned int newscore =
            bestFoundry(script, score, request.styleStrategy,
                        test.family, foundry_name, styleKey, request.pixelSize, pitch,
                        &test, request.styleName);
        if (test.foundry == nullptr && !foundry_name.isEmpty()) {
            // the specific foundry was not found, so look for
            // any foundry matching our requirements
            newscore = bestFoundry(script, score, request.styleStrategy, test.family,
                                   QString(), styleKey, request.pixelSize,
                                   pitch, &test, request.styleName);
        }

        if (newscore < score) {
            result = x;
            score = newscore;
            *desc = test;
        }
        if (newscore < 10) // xlfd instead of FT... just accept it
            break;
    }

    if (resultingScore != nullptr)
        *resultingScore = score;

    return result;
}

static QString styleStringHelper(int weight, QFont::Style style)
{
    QString result;
    if (weight > QFont::Normal) {
        if (weight >= QFont::Black)
            result = QCoreApplication::translate("QFontDatabase", "Black");
        else if (weight >= QFont::ExtraBold)
            result = QCoreApplication::translate("QFontDatabase", "Extra Bold");
        else if (weight >= QFont::Bold)
            result = QCoreApplication::translate("QFontDatabase", "Bold");
        else if (weight >= QFont::DemiBold)
            result = QCoreApplication::translate("QFontDatabase", "Demi Bold");
        else if (weight >= QFont::Medium)
            result = QCoreApplication::translate("QFontDatabase", "Medium", "The Medium font weight");
    } else {
        if (weight <= QFont::Thin)
            result = QCoreApplication::translate("QFontDatabase", "Thin");
        else if (weight <= QFont::ExtraLight)
            result = QCoreApplication::translate("QFontDatabase", "Extra Light");
        else if (weight <= QFont::Light)
            result = QCoreApplication::translate("QFontDatabase", "Light");
    }

    if (style == QFont::StyleItalic)
        result += u' ' + QCoreApplication::translate("QFontDatabase", "Italic");
    else if (style == QFont::StyleOblique)
        result += u' ' + QCoreApplication::translate("QFontDatabase", "Oblique");

    if (result.isEmpty())
        result = QCoreApplication::translate("QFontDatabase", "Normal", "The Normal or Regular font weight");

    return result.simplified();
}

/*!
    Returns a string that describes the style of the \a font. For
    example, "Bold Italic", "Bold", "Italic" or "Normal". An empty
    string may be returned.
*/
QString QFontDatabase::styleString(const QFont &font)
{
    return font.styleName().isEmpty() ? styleStringHelper(font.weight(), font.style())
                                      : font.styleName();
}

/*!
    Returns a string that describes the style of the \a fontInfo. For
    example, "Bold Italic", "Bold", "Italic" or "Normal". An empty
    string may be returned.
*/
QString QFontDatabase::styleString(const QFontInfo &fontInfo)
{
    return fontInfo.styleName().isEmpty() ? styleStringHelper(fontInfo.weight(), fontInfo.style())
                                          : fontInfo.styleName();
}


/*!
    \class QFontDatabase
    \threadsafe
    \inmodule QtGui

    \brief The QFontDatabase class provides information about the fonts available in the underlying window system.

    \ingroup appearance

    The most common uses of this class are to query the database for
    the list of font families() and for the pointSizes() and styles()
    that are available for each family. An alternative to pointSizes()
    is smoothSizes() which returns the sizes at which a given family
    and style will look attractive.

    If the font family is available from two or more foundries the
    foundry name is included in the family name; for example:
    "Helvetica [Adobe]" and "Helvetica [Cronyx]". When you specify a
    family, you can either use the old hyphenated "foundry-family"
    format or the bracketed "family [foundry]" format; for example:
    "Cronyx-Helvetica" or "Helvetica [Cronyx]". If the family has a
    foundry it is always returned using the bracketed format, as is
    the case with the value returned by families().

    The font() function returns a QFont given a family, style and
    point size.

    A family and style combination can be checked to see if it is
    italic() or bold(), and to retrieve its weight(). Similarly we can
    call isBitmapScalable(), isSmoothlyScalable(), isScalable() and
    isFixedPitch().

    Use the styleString() to obtain a text version of a style.

    The QFontDatabase class provides some helper functions, for
    example, standardSizes(). You can retrieve the description of a
    writing system using writingSystemName(), and a sample of
    characters in a writing system with writingSystemSample().

    Example:

    \snippet qfontdatabase/qfontdatabase_snippets.cpp 0

    This example gets the list of font families, the list of
    styles for each family, and the point sizes that are available for
    each combination of family and style, displaying this information
    in a tree view.

    \sa QFont, QFontInfo, QFontMetrics
*/

/*!
    \fn QFontDatabase::QFontDatabase()
    \deprecated [6.0] Call the class methods as static functions instead.

    Creates a font database object.
*/

/*!
    \enum QFontDatabase::WritingSystem

    \value Any
    \value Latin
    \value Greek
    \value Cyrillic
    \value Armenian
    \value Hebrew
    \value Arabic
    \value Syriac
    \value Thaana
    \value Devanagari
    \value Bengali
    \value Gurmukhi
    \value Gujarati
    \value Oriya
    \value Tamil
    \value Telugu
    \value Kannada
    \value Malayalam
    \value Sinhala
    \value Thai
    \value Lao
    \value Tibetan
    \value Myanmar
    \value Georgian
    \value Khmer
    \value SimplifiedChinese
    \value TraditionalChinese
    \value Japanese
    \value Korean
    \value Vietnamese
    \value Symbol
    \value Other (the same as Symbol)
    \value Ogham
    \value Runic
    \value Nko

    \omitvalue WritingSystemsCount
*/

/*!
    \enum QFontDatabase::SystemFont

    \value GeneralFont              The default system font.
    \value FixedFont                The fixed font that the system recommends.
    \value TitleFont                The system standard font for titles.
    \value SmallestReadableFont     The smallest readable system font.

    \since 5.2
*/

/*!
    \class QFontDatabasePrivate
    \internal

    Singleton implementation of the public QFontDatabase APIs,
    accessed through QFontDatabasePrivate::instance().

    The database is organized in multiple levels:

      - QFontDatabasePrivate::families
        - QtFontFamily::foundries
          - QtFontFoundry::styles
            - QtFontStyle::sizes
              - QtFontSize::pixelSize

    The font database is the single source of truth when doing
    font matching, so the database must be sufficiently filled
    before attempting a match.

    The database is populated (filled) from two sources:

     1. The system (platform's) view of the available fonts

        Initiated via QFontDatabasePrivate::populateFontDatabase().

        a. Can be registered lazily by family only, by calling
           QPlatformFontDatabase::registerFontFamily(), and later
           populated via QPlatformFontDatabase::populateFamily().

        b. Or fully registered with all styles, by calling
           QPlatformFontDatabase::registerFont().

     2. The fonts registered by the application via Qt APIs

        Initiated via QFontDatabase::addApplicationFont() and
        QFontDatabase::addApplicationFontFromData().

        Application fonts are always fully registered when added.

    Fonts can be added at any time, so the database may grow even
    after QFontDatabasePrivate::populateFontDatabase() has been
    completed.

    The database does not support granular removal of fonts,
    so if the system fonts change, or an application font is
    removed, the font database will be cleared and then filled
    from scratch, via QFontDatabasePrivate:invalidate() and
    QFontDatabasePrivate::ensureFontDatabase().
*/

/*!
    \internal

    Initializes the font database if necessary and returns its
    pointer. Mutex lock must be held when calling this function.
*/
QFontDatabasePrivate *QFontDatabasePrivate::ensureFontDatabase()
{
    auto *d = QFontDatabasePrivate::instance();
    if (!d->populated) {
        // The font database may have been partially populated, but to ensure
        // we can answer queries for any platform- or user-provided family we
        // need to fully populate it now.
        qCDebug(lcFontDb) << "Populating font database";

        if (Q_UNLIKELY(qGuiApp == nullptr || QGuiApplicationPrivate::platformIntegration() == nullptr))
            qFatal("QFontDatabase: Must construct a QGuiApplication before accessing QFontDatabase");

        auto *platformFontDatabase = QGuiApplicationPrivate::platformIntegration()->fontDatabase();
        platformFontDatabase->populateFontDatabase();

        for (int i = 0; i < d->applicationFonts.size(); i++) {
            auto *font = &d->applicationFonts[i];
            if (!font->isNull() && !font->isPopulated())
                platformFontDatabase->addApplicationFont(font->data, font->fileName, font);
        }

        // Note: Both application fonts and platform fonts may be added
        // after this initial population, so the only thing we are tracking
        // is whether we've done our part in ensuring a filled font database.
        d->populated = true;
    }
    return d;
}

/*!
    Returns a sorted list of the available writing systems. This is
    list generated from information about all installed fonts on the
    system.

    \sa families()
*/
QList<QFontDatabase::WritingSystem> QFontDatabase::writingSystems()
{
    QMutexLocker locker(fontDatabaseMutex());
    QFontDatabasePrivate *d = QFontDatabasePrivate::ensureFontDatabase();

    quint64 writingSystemsFound = 0;
    static_assert(WritingSystemsCount < 64);

    for (int i = 0; i < d->count; ++i) {
        QtFontFamily *family = d->families[i];
        if (!family->ensurePopulated())
            continue;

        if (family->count == 0)
            continue;
        for (uint x = Latin; x < uint(WritingSystemsCount); ++x) {
            if (family->writingSystems[x] & QtFontFamily::Supported)
                writingSystemsFound |= quint64(1) << x;
        }
    }

    // mutex protection no longer needed - just working on local data now:
    locker.unlock();

    QList<WritingSystem> list;
    list.reserve(qPopulationCount(writingSystemsFound));
    for (uint x = Latin ; x < uint(WritingSystemsCount); ++x) {
        if (writingSystemsFound & (quint64(1) << x))
            list.push_back(WritingSystem(x));
    }
    return list;
}


/*!
    Returns a sorted list of the writing systems supported by a given
    font \a family.

    \sa families()
*/
QList<QFontDatabase::WritingSystem> QFontDatabase::writingSystems(const QString &family)
{
    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());
    QFontDatabasePrivate *d = QFontDatabasePrivate::ensureFontDatabase();

    QList<WritingSystem> list;
    QtFontFamily *f = d->family(familyName);
    if (!f || f->count == 0)
        return list;

    for (int x = Latin; x < WritingSystemsCount; ++x) {
        const WritingSystem writingSystem = WritingSystem(x);
        if (f->writingSystems[writingSystem] & QtFontFamily::Supported)
            list.append(writingSystem);
    }
    return list;
}


/*!
    Returns a sorted list of the available font families which support
    the \a writingSystem.

    If a family exists in several foundries, the returned name for
    that font is in the form "family [foundry]". Examples: "Times
    [Adobe]", "Times [Cronyx]", "Palatino".

    \sa writingSystems()
*/
QStringList QFontDatabase::families(WritingSystem writingSystem)
{
    QMutexLocker locker(fontDatabaseMutex());
    QFontDatabasePrivate *d = QFontDatabasePrivate::ensureFontDatabase();

    QStringList flist;
    for (int i = 0; i < d->count; i++) {
        QtFontFamily *f = d->families[i];
        if (f->populated && f->count == 0)
            continue;
        if (writingSystem != Any) {
            if (!f->ensurePopulated())
                continue;
            if (f->writingSystems[writingSystem] != QtFontFamily::Supported)
                continue;
        }
        if (!f->populated || f->count == 1) {
            flist.append(f->name);
        } else {
            for (int j = 0; j < f->count; j++) {
                QString str = f->name;
                QString foundry = f->foundries[j]->name;
                if (!foundry.isEmpty()) {
                    str += " ["_L1;
                    str += foundry;
                    str += u']';
                }
                flist.append(str);
            }
        }
    }
    return flist;
}

/*!
    Returns a list of the styles available for the font family \a
    family. Some example styles: "Light", "Light Italic", "Bold",
    "Oblique", "Demi". The list may be empty.

    \sa families()
*/
QStringList QFontDatabase::styles(const QString &family)
{
    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());
    QFontDatabasePrivate *d = QFontDatabasePrivate::ensureFontDatabase();

    QStringList l;
    QtFontFamily *f = d->family(familyName);
    if (!f)
        return l;

    QtFontFoundry allStyles(foundryName);
    for (int j = 0; j < f->count; j++) {
        QtFontFoundry *foundry = f->foundries[j];
        if (foundryName.isEmpty() || foundry->name.compare(foundryName, Qt::CaseInsensitive) == 0) {
            for (int k = 0; k < foundry->count; k++) {
                QtFontStyle::Key ke(foundry->styles[k]->key);
                ke.stretch = 0;
                allStyles.style(ke, foundry->styles[k]->styleName, true);
            }
        }
    }

    l.reserve(allStyles.count);
    for (int i = 0; i < allStyles.count; i++) {
        l.append(allStyles.styles[i]->styleName.isEmpty() ?
                 styleStringHelper(allStyles.styles[i]->key.weight,
                                   (QFont::Style)allStyles.styles[i]->key.style) :
                 allStyles.styles[i]->styleName);
    }
    return l;
}

/*!
    Returns \c true if the font that has family \a family and style \a
    style is fixed pitch; otherwise returns \c false.
*/

bool QFontDatabase::isFixedPitch(const QString &family,
                                 const QString &style)
{
    Q_UNUSED(style);

    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());
    QFontDatabasePrivate *d = QFontDatabasePrivate::ensureFontDatabase();

    QtFontFamily *f = d->family(familyName);
    return (f && f->fixedPitch);
}

/*!
    Returns \c true if the font that has family \a family and style \a
    style is a scalable bitmap font; otherwise returns \c false. Scaling
    a bitmap font usually produces an unattractive hardly readable
    result, because the pixels of the font are scaled. If you need to
    scale a bitmap font it is better to scale it to one of the fixed
    sizes returned by smoothSizes().

    \sa isScalable(), isSmoothlyScalable()
*/
bool QFontDatabase::isBitmapScalable(const QString &family,
                                      const QString &style)
{
    bool bitmapScalable = false;
    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());
    QFontDatabasePrivate *d = QFontDatabasePrivate::ensureFontDatabase();

    QtFontFamily *f = d->family(familyName);
    if (!f) return bitmapScalable;

    QtFontStyle::Key styleKey(style);
    for (int j = 0; j < f->count; j++) {
        QtFontFoundry *foundry = f->foundries[j];
        if (foundryName.isEmpty() || foundry->name.compare(foundryName, Qt::CaseInsensitive) == 0) {
            for (int k = 0; k < foundry->count; k++)
                if ((style.isEmpty() ||
                     foundry->styles[k]->styleName == style ||
                     foundry->styles[k]->key == styleKey)
                    && foundry->styles[k]->bitmapScalable && !foundry->styles[k]->smoothScalable) {
                    bitmapScalable = true;
                    goto end;
                }
        }
    }
 end:
    return bitmapScalable;
}


/*!
    Returns \c true if the font that has family \a family and style \a
    style is smoothly scalable; otherwise returns \c false. If this
    function returns \c true, it's safe to scale this font to any size,
    and the result will always look attractive.

    \sa isScalable(), isBitmapScalable()
*/
bool QFontDatabase::isSmoothlyScalable(const QString &family, const QString &style)
{
    bool smoothScalable = false;
    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());
    QFontDatabasePrivate *d = QFontDatabasePrivate::ensureFontDatabase();

    QtFontFamily *f = d->family(familyName);
    if (!f) {
        for (int i = 0; i < d->count; i++) {
            if (d->families[i]->matchesFamilyName(familyName)) {
                f = d->families[i];
                if (f->ensurePopulated())
                    break;
            }
        }
    }
    if (!f) return smoothScalable;

    const QtFontStyle::Key styleKey(style);
    for (int j = 0; j < f->count; j++) {
        QtFontFoundry *foundry = f->foundries[j];
        if (foundryName.isEmpty() || foundry->name.compare(foundryName, Qt::CaseInsensitive) == 0) {
            for (int k = 0; k < foundry->count; k++) {
                const QtFontStyle *fontStyle = foundry->styles[k];
                smoothScalable =
                        fontStyle->smoothScalable
                        && ((style.isEmpty()
                             || fontStyle->styleName == style
                             || fontStyle->key == styleKey)
                            || (fontStyle->styleName.isEmpty()
                                && style == styleStringHelper(fontStyle->key.weight,
                                                              QFont::Style(fontStyle->key.style))));
                if (smoothScalable)
                    goto end;
            }
        }
    }
 end:
    return smoothScalable;
}

/*!
    Returns \c true if the font that has family \a family and style \a
    style is scalable; otherwise returns \c false.

    \sa isBitmapScalable(), isSmoothlyScalable()
*/
bool  QFontDatabase::isScalable(const QString &family,
                                 const QString &style)
{
    QMutexLocker locker(fontDatabaseMutex());
    if (isSmoothlyScalable(family, style))
        return true;
    return isBitmapScalable(family, style);
}


/*!
    Returns a list of the point sizes available for the font that has
    family \a family and style \a styleName. The list may be empty.

    \sa smoothSizes(), standardSizes()
*/
QList<int> QFontDatabase::pointSizes(const QString &family,
                                     const QString &styleName)
{
    if (QGuiApplicationPrivate::platformIntegration()->fontDatabase()->fontsAlwaysScalable())
        return standardSizes();

    bool smoothScalable = false;
    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());
    QFontDatabasePrivate *d = QFontDatabasePrivate::ensureFontDatabase();

    QList<int> sizes;

    QtFontFamily *fam = d->family(familyName);
    if (!fam) return sizes;


    const int dpi = qt_defaultDpiY(); // embedded

    QtFontStyle::Key styleKey(styleName);
    for (int j = 0; j < fam->count; j++) {
        QtFontFoundry *foundry = fam->foundries[j];
        if (foundryName.isEmpty() || foundry->name.compare(foundryName, Qt::CaseInsensitive) == 0) {
            QtFontStyle *style = foundry->style(styleKey, styleName);
            if (!style) continue;

            if (style->smoothScalable) {
                smoothScalable = true;
                goto end;
            }
            for (int l = 0; l < style->count; l++) {
                const QtFontSize *size = style->pixelSizes + l;

                if (size->pixelSize != 0 && size->pixelSize != SMOOTH_SCALABLE) {
                    const int pointSize = qRound(size->pixelSize * 72.0 / dpi);
                    if (! sizes.contains(pointSize))
                        sizes.append(pointSize);
                }
            }
        }
    }
 end:
    if (smoothScalable)
        return standardSizes();

    std::sort(sizes.begin(), sizes.end());
    return sizes;
}

/*!
    Returns a QFont object that has family \a family, style \a style
    and point size \a pointSize. If no matching font could be created,
    a QFont object that uses the application's default font is
    returned.
*/
QFont QFontDatabase::font(const QString &family, const QString &style,
                          int pointSize)
{
    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);
    QMutexLocker locker(fontDatabaseMutex());
    QFontDatabasePrivate *d = QFontDatabasePrivate::ensureFontDatabase();

    QtFontFoundry allStyles(foundryName);
    QtFontFamily *f = d->family(familyName);
    if (!f) return QGuiApplication::font();

    for (int j = 0; j < f->count; j++) {
        QtFontFoundry *foundry = f->foundries[j];
        if (foundryName.isEmpty() || foundry->name.compare(foundryName, Qt::CaseInsensitive) == 0) {
            for (int k = 0; k < foundry->count; k++)
                allStyles.style(foundry->styles[k]->key, foundry->styles[k]->styleName, true);
        }
    }

    QtFontStyle::Key styleKey(style);
    QtFontStyle *s = bestStyle(&allStyles, styleKey, style);

    if (!s) // no styles found?
        return QGuiApplication::font();

    QFont fnt(QStringList{family}, pointSize, s->key.weight);
    fnt.setStyle((QFont::Style)s->key.style);
    if (!s->styleName.isEmpty())
        fnt.setStyleName(s->styleName);
    return fnt;
}


/*!
    Returns the point sizes of a font that has family \a family and
    style \a styleName that will look attractive. The list may be empty.
    For non-scalable fonts and bitmap scalable fonts, this function
    is equivalent to pointSizes().

  \sa pointSizes(), standardSizes()
*/
QList<int> QFontDatabase::smoothSizes(const QString &family,
                                            const QString &styleName)
{
    if (QGuiApplicationPrivate::platformIntegration()->fontDatabase()->fontsAlwaysScalable())
        return standardSizes();

    bool smoothScalable = false;
    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());
    QFontDatabasePrivate *d = QFontDatabasePrivate::ensureFontDatabase();

    QList<int> sizes;

    QtFontFamily *fam = d->family(familyName);
    if (!fam)
        return sizes;

    const int dpi = qt_defaultDpiY(); // embedded

    QtFontStyle::Key styleKey(styleName);
    for (int j = 0; j < fam->count; j++) {
        QtFontFoundry *foundry = fam->foundries[j];
        if (foundryName.isEmpty() || foundry->name.compare(foundryName, Qt::CaseInsensitive) == 0) {
            QtFontStyle *style = foundry->style(styleKey, styleName);
            if (!style) continue;

            if (style->smoothScalable) {
                smoothScalable = true;
                goto end;
            }
            for (int l = 0; l < style->count; l++) {
                const QtFontSize *size = style->pixelSizes + l;

                if (size->pixelSize != 0 && size->pixelSize != SMOOTH_SCALABLE) {
                    const int pointSize = qRound(size->pixelSize * 72.0 / dpi);
                    if (! sizes.contains(pointSize))
                        sizes.append(pointSize);
                }
            }
        }
    }
 end:
    if (smoothScalable)
        return QFontDatabase::standardSizes();

    std::sort(sizes.begin(), sizes.end());
    return sizes;
}


/*!
    Returns a list of standard font sizes.

    \sa smoothSizes(), pointSizes()
*/
QList<int> QFontDatabase::standardSizes()
{
    return QGuiApplicationPrivate::platformIntegration()->fontDatabase()->standardSizes();
}


/*!
    Returns \c true if the font that has family \a family and style \a
    style is italic; otherwise returns \c false.

    \sa weight(), bold()
*/
bool QFontDatabase::italic(const QString &family, const QString &style)
{
    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());
    QFontDatabasePrivate *d = QFontDatabasePrivate::ensureFontDatabase();

    QtFontFoundry allStyles(foundryName);
    QtFontFamily *f = d->family(familyName);
    if (!f) return false;

    for (int j = 0; j < f->count; j++) {
        QtFontFoundry *foundry = f->foundries[j];
        if (foundryName.isEmpty() || foundry->name.compare(foundryName, Qt::CaseInsensitive) == 0) {
            for (int k = 0; k < foundry->count; k++)
                allStyles.style(foundry->styles[k]->key, foundry->styles[k]->styleName, true);
        }
    }

    QtFontStyle::Key styleKey(style);
    QtFontStyle *s = allStyles.style(styleKey, style);
    return s && s->key.style == QFont::StyleItalic;
}


/*!
    Returns \c true if the font that has family \a family and style \a
    style is bold; otherwise returns \c false.

    \sa italic(), weight()
*/
bool QFontDatabase::bold(const QString &family,
                          const QString &style)
{
    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());
    QFontDatabasePrivate *d = QFontDatabasePrivate::ensureFontDatabase();

    QtFontFoundry allStyles(foundryName);
    QtFontFamily *f = d->family(familyName);
    if (!f) return false;

    for (int j = 0; j < f->count; j++) {
        QtFontFoundry *foundry = f->foundries[j];
        if (foundryName.isEmpty() ||
            foundry->name.compare(foundryName, Qt::CaseInsensitive) == 0) {
            for (int k = 0; k < foundry->count; k++)
                allStyles.style(foundry->styles[k]->key, foundry->styles[k]->styleName, true);
        }
    }

    QtFontStyle::Key styleKey(style);
    QtFontStyle *s = allStyles.style(styleKey, style);
    return s && s->key.weight >= QFont::Bold;
}


/*!
    Returns the weight of the font that has family \a family and style
    \a style. If there is no such family and style combination,
    returns -1.

    \sa italic(), bold()
*/
int QFontDatabase::weight(const QString &family,
                           const QString &style)
{
    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());
    QFontDatabasePrivate *d = QFontDatabasePrivate::ensureFontDatabase();

    QtFontFoundry allStyles(foundryName);
    QtFontFamily *f = d->family(familyName);
    if (!f) return -1;

    for (int j = 0; j < f->count; j++) {
        QtFontFoundry *foundry = f->foundries[j];
        if (foundryName.isEmpty() ||
            foundry->name.compare(foundryName, Qt::CaseInsensitive) == 0) {
            for (int k = 0; k < foundry->count; k++)
                allStyles.style(foundry->styles[k]->key, foundry->styles[k]->styleName, true);
        }
    }

    QtFontStyle::Key styleKey(style);
    QtFontStyle *s = allStyles.style(styleKey, style);
    return s ? s->key.weight : -1;
}


/*! \internal */
bool QFontDatabase::hasFamily(const QString &family)
{
    QString parsedFamily, foundry;
    parseFontName(family, foundry, parsedFamily);
    const QString familyAlias = QFontDatabasePrivate::resolveFontFamilyAlias(parsedFamily);

    QMutexLocker locker(fontDatabaseMutex());
    QFontDatabasePrivate *d = QFontDatabasePrivate::ensureFontDatabase();

    for (int i = 0; i < d->count; i++) {
        QtFontFamily *f = d->families[i];
        if (f->populated && f->count == 0)
            continue;
        if (familyAlias.compare(f->name, Qt::CaseInsensitive) == 0)
            return true;
    }

    return false;
}


/*!
    \since 5.5

    Returns \c true if and only if the \a family font family is private.

    This happens, for instance, on \macos and iOS, where the system UI fonts are not
    accessible to the user. For completeness, QFontDatabase::families() returns all
    font families, including the private ones. You should use this function if you
    are developing a font selection control in order to keep private fonts hidden.

    \sa families()
*/
bool QFontDatabase::isPrivateFamily(const QString &family)
{
    return QGuiApplicationPrivate::platformIntegration()->fontDatabase()->isPrivateFontFamily(family);
}


/*!
    Returns the names the \a writingSystem (e.g. for displaying to the
    user in a dialog).
*/
QString QFontDatabase::writingSystemName(WritingSystem writingSystem)
{
    const char *name = nullptr;
    switch (writingSystem) {
    case Any:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Any");
        break;
    case Latin:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Latin");
        break;
    case Greek:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Greek");
        break;
    case Cyrillic:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Cyrillic");
        break;
    case Armenian:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Armenian");
        break;
    case Hebrew:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Hebrew");
        break;
    case Arabic:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Arabic");
        break;
    case Syriac:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Syriac");
        break;
    case Thaana:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Thaana");
        break;
    case Devanagari:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Devanagari");
        break;
    case Bengali:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Bengali");
        break;
    case Gurmukhi:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Gurmukhi");
        break;
    case Gujarati:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Gujarati");
        break;
    case Oriya:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Oriya");
        break;
    case Tamil:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Tamil");
        break;
    case Telugu:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Telugu");
        break;
    case Kannada:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Kannada");
        break;
    case Malayalam:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Malayalam");
        break;
    case Sinhala:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Sinhala");
        break;
    case Thai:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Thai");
        break;
    case Lao:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Lao");
        break;
    case Tibetan:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Tibetan");
        break;
    case Myanmar:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Myanmar");
        break;
    case Georgian:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Georgian");
        break;
    case Khmer:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Khmer");
        break;
    case SimplifiedChinese:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Simplified Chinese");
        break;
    case TraditionalChinese:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Traditional Chinese");
        break;
    case Japanese:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Japanese");
        break;
    case Korean:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Korean");
        break;
    case Vietnamese:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Vietnamese");
        break;
    case Symbol:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Symbol");
        break;
    case Ogham:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Ogham");
        break;
    case Runic:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Runic");
        break;
    case Nko:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "N'Ko");
        break;
    default:
        Q_ASSERT_X(false, "QFontDatabase::writingSystemName", "invalid 'writingSystem' parameter");
        break;
    }
    return QCoreApplication::translate("QFontDatabase", name);
}

/*!
    Returns a string with sample characters from \a writingSystem.
*/
QString QFontDatabase::writingSystemSample(WritingSystem writingSystem)
{
    return [&]() -> QStringView {
        switch (writingSystem) {
        case QFontDatabase::Any:
        case QFontDatabase::Symbol:
            // show only ascii characters
            return u"AaBbzZ";
        case QFontDatabase::Latin:
            // This is cheating... we only show latin-1 characters so that we don't
            // end up loading lots of fonts - at least on X11...
            return u"Aa\x00C3\x00E1Zz";
        case QFontDatabase::Greek:
            return u"\x0393\x03B1\x03A9\x03C9";
        case QFontDatabase::Cyrillic:
            return u"\x0414\x0434\x0436\x044f";
        case QFontDatabase::Armenian:
            return u"\x053f\x054f\x056f\x057f";
        case QFontDatabase::Hebrew:
            return u"\x05D0\x05D1\x05D2\x05D3";
        case QFontDatabase::Arabic:
            return u"\x0623\x0628\x062C\x062F\x064A\x0629\x0020\x0639\x0631\x0628\x064A\x0629";
        case QFontDatabase::Syriac:
            return u"\x0715\x0725\x0716\x0726";
        case QFontDatabase::Thaana:
            return u"\x0784\x0794\x078c\x078d";
        case QFontDatabase::Devanagari:
            return u"\x0905\x0915\x0925\x0935";
        case QFontDatabase::Bengali:
            return u"\x0986\x0996\x09a6\x09b6";
        case QFontDatabase::Gurmukhi:
            return u"\x0a05\x0a15\x0a25\x0a35";
        case QFontDatabase::Gujarati:
            return u"\x0a85\x0a95\x0aa5\x0ab5";
        case QFontDatabase::Oriya:
            return u"\x0b06\x0b16\x0b2b\x0b36";
        case QFontDatabase::Tamil:
            return u"\x0b89\x0b99\x0ba9\x0bb9";
        case QFontDatabase::Telugu:
            return u"\x0c05\x0c15\x0c25\x0c35";
        case QFontDatabase::Kannada:
            return u"\x0c85\x0c95\x0ca5\x0cb5";
        case QFontDatabase::Malayalam:
            return u"\x0d05\x0d15\x0d25\x0d35";
        case QFontDatabase::Sinhala:
            return u"\x0d90\x0da0\x0db0\x0dc0";
        case QFontDatabase::Thai:
            return u"\x0e02\x0e12\x0e22\x0e32";
        case QFontDatabase::Lao:
            return u"\x0e8d\x0e9d\x0ead\x0ebd";
        case QFontDatabase::Tibetan:
            return u"\x0f00\x0f01\x0f02\x0f03";
        case QFontDatabase::Myanmar:
            return u"\x1000\x1001\x1002\x1003";
        case QFontDatabase::Georgian:
            return u"\x10a0\x10b0\x10c0\x10d0";
        case QFontDatabase::Khmer:
            return u"\x1780\x1790\x17b0\x17c0";
        case QFontDatabase::SimplifiedChinese:
            return u"\x4e2d\x6587\x8303\x4f8b";
        case QFontDatabase::TraditionalChinese:
            return u"\x4e2d\x6587\x7bc4\x4f8b";
        case QFontDatabase::Japanese:
            return u"\x30b5\x30f3\x30d7\x30eb\x3067\x3059";
        case QFontDatabase::Korean:
            return u"\xac00\xac11\xac1a\xac2f";
        case QFontDatabase::Vietnamese:
            return u"\x1ED7\x1ED9\x1ED1\x1ED3";
        case QFontDatabase::Ogham:
            return u"\x1681\x1682\x1683\x1684";
        case QFontDatabase::Runic:
            return u"\x16a0\x16a1\x16a2\x16a3";
        case QFontDatabase::Nko:
            return u"\x7ca\x7cb\x7cc\x7cd";
        default:
            return nullptr;
        }
    }().toString();
}

void QFontDatabasePrivate::parseFontName(const QString &name, QString &foundry, QString &family)
{
    QT_PREPEND_NAMESPACE(parseFontName)(name, foundry, family);
}

// used from qfontengine_ft.cpp
Q_GUI_EXPORT QByteArray qt_fontdata_from_index(int index)
{
    QMutexLocker locker(fontDatabaseMutex());
    return QFontDatabasePrivate::instance()->applicationFonts.value(index).data;
}

int QFontDatabasePrivate::addAppFont(const QByteArray &fontData, const QString &fileName)
{
    QFontDatabasePrivate::ApplicationFont font;
    font.data = fontData;
    font.fileName = fileName;

    Q_TRACE(QFontDatabasePrivate_addAppFont, fileName);

    int i;
    for (i = 0; i < applicationFonts.size(); ++i)
        if (applicationFonts.at(i).isNull())
            break;
    if (i >= applicationFonts.size()) {
        applicationFonts.append(ApplicationFont());
        i = applicationFonts.size() - 1;
    }

    if (font.fileName.isEmpty() && !fontData.isEmpty())
        font.fileName = ":qmemoryfonts/"_L1 + QString::number(i);

    auto *platformFontDatabase = QGuiApplicationPrivate::platformIntegration()->fontDatabase();
    platformFontDatabase->addApplicationFont(font.data, font.fileName, &font);
    if (font.properties.isEmpty())
        return -1;

    applicationFonts[i] = font;

    // The font cache may have cached lookups for the font that was now
    // loaded, so it has to be flushed.
    QFontCache::instance()->clear();

    emit qApp->fontDatabaseChanged();

    return i;
}

bool QFontDatabasePrivate::isApplicationFont(const QString &fileName)
{
    for (int i = 0; i < applicationFonts.size(); ++i)
        if (applicationFonts.at(i).fileName == fileName)
            return true;
    return false;
}

/*!
    \since 4.2

    Loads the font from the file specified by \a fileName and makes it available to
    the application. An ID is returned that can be used to remove the font again
    with removeApplicationFont() or to retrieve the list of family names contained
    in the font.

//! [add-application-font-doc]
    The function returns -1 if the font could not be loaded.

    Currently only TrueType fonts, TrueType font collections, and OpenType fonts are
    supported.
//! [add-application-font-doc]

    \sa addApplicationFontFromData(), applicationFontFamilies(), removeApplicationFont()
*/
int QFontDatabase::addApplicationFont(const QString &fileName)
{
    QByteArray data;
    if (!QFileInfo(fileName).isNativePath()) {
        QFile f(fileName);
        if (!f.open(QIODevice::ReadOnly))
            return -1;

        Q_TRACE(QFontDatabase_addApplicationFont, fileName);

        data = f.readAll();
    }
    QMutexLocker locker(fontDatabaseMutex());
    return QFontDatabasePrivate::instance()->addAppFont(data, fileName);
}

/*!
    \since 4.2

    Loads the font from binary data specified by \a fontData and makes it available to
    the application. An ID is returned that can be used to remove the font again
    with removeApplicationFont() or to retrieve the list of family names contained
    in the font.

    \include qfontdatabase.cpp add-application-font-doc

    \sa addApplicationFont(), applicationFontFamilies(), removeApplicationFont()
*/
int QFontDatabase::addApplicationFontFromData(const QByteArray &fontData)
{
    QMutexLocker locker(fontDatabaseMutex());
    return QFontDatabasePrivate::instance()->addAppFont(fontData, QString() /* fileName */);
}

/*!
    \since 4.2

    Returns a list of font families for the given application font identified by
    \a id.

    \sa addApplicationFont(), addApplicationFontFromData()
*/
QStringList QFontDatabase::applicationFontFamilies(int id)
{
    QMutexLocker locker(fontDatabaseMutex());
    auto *d = QFontDatabasePrivate::instance();

    QStringList ret;
    ret.reserve(d->applicationFonts.value(id).properties.size());

    for (const auto &properties : d->applicationFonts.value(id).properties)
        ret.append(properties.familyName);

    return ret;
}

/*!
    \since 5.2

    Returns the most adequate font for a given \a type case for proper integration
    with the system's look and feel.

    \sa QGuiApplication::font()
*/

QFont QFontDatabase::systemFont(QFontDatabase::SystemFont type)
{
    const QFont *font = nullptr;
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme()) {
        switch (type) {
            case GeneralFont:
                font = theme->font(QPlatformTheme::SystemFont);
                break;
            case FixedFont:
                font = theme->font(QPlatformTheme::FixedFont);
                break;
            case TitleFont:
                font = theme->font(QPlatformTheme::TitleBarFont);
                break;
            case SmallestReadableFont:
                font = theme->font(QPlatformTheme::MiniFont);
                break;
        }
    }

    if (font)
        return *font;
    else if (QPlatformIntegration *integration = QGuiApplicationPrivate::platformIntegration())
        return integration->fontDatabase()->defaultFont();
    else
        return QFont();
}

/*!
    \fn bool QFontDatabase::removeApplicationFont(int id)
    \since 4.2

    Removes the previously loaded application font identified by \a
    id. Returns \c true if unloading of the font succeeded; otherwise
    returns \c false.

    \sa removeAllApplicationFonts(), addApplicationFont(),
        addApplicationFontFromData()
*/
bool QFontDatabase::removeApplicationFont(int handle)
{
    QMutexLocker locker(fontDatabaseMutex());

    auto *db = QFontDatabasePrivate::instance();
    if (handle < 0 || handle >= db->applicationFonts.size())
        return false;

    db->applicationFonts[handle] = QFontDatabasePrivate::ApplicationFont();

    db->invalidate();
    return true;
}

/*!
    \fn bool QFontDatabase::removeAllApplicationFonts()
    \since 4.2

    Removes all application-local fonts previously added using addApplicationFont()
    and addApplicationFontFromData().

    Returns \c true if unloading of the fonts succeeded; otherwise
    returns \c false.

    \sa removeApplicationFont(), addApplicationFont(), addApplicationFontFromData()
*/
bool QFontDatabase::removeAllApplicationFonts()
{
    QMutexLocker locker(fontDatabaseMutex());

    auto *db = QFontDatabasePrivate::instance();
    if (!db || db->applicationFonts.isEmpty())
        return false;

    db->applicationFonts.clear();
    db->invalidate();
    return true;
}

/*!
    \internal
*/
QFontEngine *QFontDatabasePrivate::findFont(const QFontDef &req,
                                            int script,
                                            bool preferScriptOverFamily)
{
    QMutexLocker locker(fontDatabaseMutex());
    ensureFontDatabase();

    QFontEngine *engine;

#ifdef Q_OS_WIN
    const QFontDef request = static_cast<QWindowsFontDatabaseBase *>(
                                     QGuiApplicationPrivate::platformIntegration()->fontDatabase())
                                     ->sanitizeRequest(req);
#else
    const QFontDef &request = req;
#endif

#if defined(QT_BUILD_INTERNAL)
    // For testing purpose only, emulates an exact-matching monospace font
    if (qt_enable_test_font && request.families.first() == "__Qt__Box__Engine__"_L1) {
        engine = new QTestFontEngine(request.pixelSize);
        engine->fontDef = request;
        return engine;
    }
#endif

    QFontCache *fontCache = QFontCache::instance();

    // Until we specifically asked not to, try looking for Multi font engine
    // first, the last '1' indicates that we want Multi font engine instead
    // of single ones
    bool multi = !(request.styleStrategy & QFont::NoFontMerging);
    QFontCache::Key key(request, script, multi ? 1 : 0);
    engine = fontCache->findEngine(key);
    if (engine) {
        qCDebug(lcFontMatch, "Cache hit level 1");
        return engine;
    }

    if (request.pixelSize > 0xffff) {
        // Stop absurd requests reaching the engines; pixel size is assumed to fit ushort
        qCDebug(lcFontMatch, "Rejecting request for pixel size %g2, returning box engine", double(request.pixelSize));
        return new QFontEngineBox(32); // not request.pixelSize, to avoid overflow/DOS
    }

    QString family_name, foundry_name;
    const QString requestFamily = request.families.at(0);
    parseFontName(requestFamily, foundry_name, family_name);
    QtFontDesc desc;
    QList<int> blackListed;
    unsigned int score = UINT_MAX;
    int index = match(multi ? QChar::Script_Common : script, request, family_name, foundry_name, &desc, blackListed, &score);
    if (score > 0 && QGuiApplicationPrivate::platformIntegration()->fontDatabase()->populateFamilyAliases(family_name)) {
        // We populated family aliases (e.g. localized families), so try again
        index = match(multi ? QChar::Script_Common : script, request, family_name, foundry_name, &desc, blackListed);
    }

    // If we do not find a match and NoFontMerging is set, use the requested font even if it does
    // not support the script.
    //
    // (we do this at the end to prefer foundries that support the script if they exist)
    if (index < 0 && !multi && !preferScriptOverFamily)
        index = match(QChar::Script_Common, request, family_name, foundry_name, &desc, blackListed);

    if (index >= 0) {
        QFontDef fontDef = request;
        // Don't pass empty family names to the platform font database, since it will then invoke its own matching
        // and we will be out of sync with the matched font.
        if (fontDef.families.isEmpty())
            fontDef.families = QStringList(desc.family->name);

        engine = loadEngine(script, fontDef, desc.family, desc.foundry, desc.style, desc.size);

        if (engine)
            initFontDef(desc, request, &engine->fontDef, multi);
        else
            blackListed.append(index);
    } else {
        qCDebug(lcFontMatch, "  NO MATCH FOUND\n");
    }

    if (!engine) {
        if (!requestFamily.isEmpty()) {
            QFont::StyleHint styleHint = QFont::StyleHint(request.styleHint);
            if (styleHint == QFont::AnyStyle && request.fixedPitch)
                styleHint = QFont::TypeWriter;

            QStringList fallbacks = request.fallBackFamilies
                                  + fallbacksForFamily(requestFamily,
                                                       QFont::Style(request.style),
                                                       styleHint,
                                                       QChar::Script(script));
            if (script > QChar::Script_Common)
                fallbacks += QString(); // Find the first font matching the specified script.

            for (int i = 0; !engine && i < fallbacks.size(); i++) {
                QFontDef def = request;
                def.families = QStringList(fallbacks.at(i));
                QFontCache::Key key(def, script, multi ? 1 : 0);
                engine = fontCache->findEngine(key);
                if (!engine) {
                    QtFontDesc desc;
                    do {
                        index = match(multi ? QChar::Script_Common : script, def, def.families.first(), ""_L1, &desc, blackListed);
                        if (index >= 0) {
                            QFontDef loadDef = def;
                            if (loadDef.families.isEmpty())
                                loadDef.families = QStringList(desc.family->name);
                            engine = loadEngine(script, loadDef, desc.family, desc.foundry, desc.style, desc.size);
                            if (engine)
                                initFontDef(desc, loadDef, &engine->fontDef, multi);
                            else
                                blackListed.append(index);
                        }
                    } while (index >= 0 && !engine);
                }
            }
        }

        if (!engine)
            engine = new QFontEngineBox(request.pixelSize);

        qCDebug(lcFontMatch, "returning box engine");
    }

    return engine;
}

void QFontDatabasePrivate::load(const QFontPrivate *d, int script)
{
    QFontDef req = d->request;

    if (req.pixelSize == -1) {
        req.pixelSize = std::floor(((req.pointSize * d->dpi) / 72) * 100 + 0.5) / 100;
        req.pixelSize = qRound(req.pixelSize);
    }
    if (req.pointSize < 0)
        req.pointSize = req.pixelSize*72.0/d->dpi;

    // respect the fallback families that might be passed through the request
    const QStringList fallBackFamilies = familyList(req);

    if (!d->engineData) {
        QFontCache *fontCache = QFontCache::instance();
        // look for the requested font in the engine data cache
        // note: fallBackFamilies are not respected in the EngineData cache key;
        //       join them with the primary selection family to avoid cache misses
        if (!d->request.families.isEmpty())
            req.families = fallBackFamilies;

        d->engineData = fontCache->findEngineData(req);
        if (!d->engineData) {
            // create a new one
            d->engineData = new QFontEngineData;
            fontCache->insertEngineData(req, d->engineData);
        }
        d->engineData->ref.ref();
    }

    // the cached engineData could have already loaded the engine we want
    if (d->engineData->engines[script])
        return;

    QFontEngine *fe = nullptr;

    Q_TRACE(QFontDatabase_load, req.families.join(QLatin1Char(';')), req.pointSize);

    req.fallBackFamilies = fallBackFamilies;
    if (!req.fallBackFamilies.isEmpty())
        req.families = QStringList(req.fallBackFamilies.takeFirst());

    // list of families to try
    QStringList family_list;

    if (!req.families.isEmpty()) {
        // Add primary selection
        family_list << req.families.at(0);

        // add the default family
        auto families = QGuiApplication::font().families();
        if (!families.isEmpty()) {
            QString defaultFamily = families.first();
            if (! family_list.contains(defaultFamily))
                family_list << defaultFamily;
        }

    }

    // null family means find the first font matching the specified script
    family_list << QString();

    QStringList::ConstIterator it = family_list.constBegin(), end = family_list.constEnd();
    for (; !fe && it != end; ++it) {
        req.families = QStringList(*it);

        fe = QFontDatabasePrivate::findFont(req, script);
        if (fe) {
            if (fe->type() == QFontEngine::Box && !req.families.at(0).isEmpty()) {
                if (fe->ref.loadRelaxed() == 0)
                    delete fe;
                fe = nullptr;
            } else {
                if (d->dpi > 0)
                    fe->fontDef.pointSize = qreal(double((fe->fontDef.pixelSize * 72) / d->dpi));
            }
        }

        // No need to check requested fallback families again
        req.fallBackFamilies.clear();
    }

    Q_ASSERT(fe);
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

QString QFontDatabasePrivate::resolveFontFamilyAlias(const QString &family)
{
    return QGuiApplicationPrivate::platformIntegration()->fontDatabase()->resolveFontFamilyAlias(family);
}

Q_GUI_EXPORT QStringList qt_sort_families_by_writing_system(QChar::Script script, const QStringList &families)
{
    size_t writingSystem = qt_writing_system_for_script(script);
    if (writingSystem == QFontDatabase::Any
            || writingSystem >= QFontDatabase::WritingSystemsCount) {
        return families;
    }

    auto *db = QFontDatabasePrivate::instance();
    QMultiMap<uint, QString> supported;
    for (int i = 0; i < families.size(); ++i) {
        const QString &family = families.at(i);

        QtFontFamily *testFamily = nullptr;
        for (int x = 0; x < db->count; ++x) {
            if (Q_UNLIKELY(matchFamilyName(family, db->families[x]))) {
                testFamily = db->families[x];
                if (testFamily->ensurePopulated())
                    break;
            }
        }

        uint order = i;
        if (testFamily == nullptr
              || !familySupportsWritingSystem(testFamily, writingSystem)) {
            order |= 1u << 31;
        }

        supported.insert(order, family);
    }

    return supported.values();
}

QT_END_NAMESPACE

#include "moc_qfontdatabase.cpp"

