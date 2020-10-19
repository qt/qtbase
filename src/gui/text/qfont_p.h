/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QFONT_P_H
#define QFONT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include "QtGui/qfont.h"
#include "QtCore/qmap.h"
#include "QtCore/qhash.h"
#include "QtCore/qobject.h"
#include "QtCore/qstringlist.h"
#include <QtGui/qfontdatabase.h>
#include "private/qfixed_p.h"

QT_BEGIN_NAMESPACE

// forwards
class QFontCache;
class QFontEngine;

#define QFONT_WEIGHT_MIN 1
#define QFONT_WEIGHT_MAX 1000

struct QFontDef
{
    inline QFontDef()
        : pointSize(-1.0),
          pixelSize(-1),
          styleStrategy(QFont::PreferDefault),
          stretch(QFont::AnyStretch),
          style(QFont::StyleNormal),
          hintingPreference(QFont::PreferDefaultHinting),
          styleHint(QFont::AnyStyle),
          weight(QFont::Normal),
          fixedPitch(false),
          ignorePitch(true),
          fixedPitchComputed(0),
          reserved(0)
    {
    }

    QStringList families;
    QString styleName;

    QStringList fallBackFamilies;

    qreal pointSize;
    qreal pixelSize;

    // Note: Variable ordering matters to make sure no variable overlaps two 32-bit registers.
    uint styleStrategy : 16;
    uint stretch : 12; // 0-4000
    uint style : 2;
    uint hintingPreference : 2;

    uint styleHint : 8;
    uint weight : 10; // 1-1000
    uint fixedPitch :  1;
    uint ignorePitch : 1;
    uint fixedPitchComputed : 1; // for Mac OS X only
    uint reserved : 11; // for future extensions

    bool exactMatch(const QFontDef &other) const;
    bool operator==(const QFontDef &other) const
    {
        return pixelSize == other.pixelSize
                    && weight == other.weight
                    && style == other.style
                    && stretch == other.stretch
                    && styleHint == other.styleHint
                    && styleStrategy == other.styleStrategy
                    && ignorePitch == other.ignorePitch && fixedPitch == other.fixedPitch
                    && families == other.families
                    && styleName == other.styleName
                    && hintingPreference == other.hintingPreference
                          ;
    }
    inline bool operator<(const QFontDef &other) const
    {
        if (pixelSize != other.pixelSize) return pixelSize < other.pixelSize;
        if (weight != other.weight) return weight < other.weight;
        if (style != other.style) return style < other.style;
        if (stretch != other.stretch) return stretch < other.stretch;
        if (styleHint != other.styleHint) return styleHint < other.styleHint;
        if (styleStrategy != other.styleStrategy) return styleStrategy < other.styleStrategy;
        if (families != other.families) return families < other.families;
        if (styleName != other.styleName)
            return styleName < other.styleName;
        if (hintingPreference != other.hintingPreference) return hintingPreference < other.hintingPreference;


        if (ignorePitch != other.ignorePitch) return ignorePitch < other.ignorePitch;
        if (fixedPitch != other.fixedPitch) return fixedPitch < other.fixedPitch;
        return false;
    }
};

inline size_t qHash(const QFontDef &fd, size_t seed = 0) noexcept
{
    return qHashMulti(seed,
                      qRound64(fd.pixelSize*10000), // use only 4 fractional digits
                      fd.weight,
                      fd.style,
                      fd.stretch,
                      fd.styleHint,
                      fd.styleStrategy,
                      fd.ignorePitch,
                      fd.fixedPitch,
                      fd.families,
                      fd.styleName,
                      fd.hintingPreference);
}

class QFontEngineData
{
public:
    QFontEngineData();
    ~QFontEngineData();

    QAtomicInt ref;
    const int fontCacheId;

    QFontEngine *engines[QChar::ScriptCount];

private:
    Q_DISABLE_COPY_MOVE(QFontEngineData)
};


class Q_GUI_EXPORT QFontPrivate
{
public:

    QFontPrivate();
    QFontPrivate(const QFontPrivate &other);
    ~QFontPrivate();

    QFontEngine *engineForScript(int script) const;
    void alterCharForCapitalization(QChar &c) const;

    QAtomicInt ref;
    QFontDef request;
    mutable QFontEngineData *engineData;
    int dpi;

    uint underline  :  1;
    uint overline   :  1;
    uint strikeOut  :  1;
    uint kerning    :  1;
    uint capital    :  3;
    bool letterSpacingIsAbsolute : 1;

    QFixed letterSpacing;
    QFixed wordSpacing;

    mutable QFontPrivate *scFont;
    QFont smallCapsFont() const { return QFont(smallCapsFontPrivate()); }
    QFontPrivate *smallCapsFontPrivate() const;

    static QFontPrivate *get(const QFont &font)
    {
        return font.d.data();
    }

    void resolve(uint mask, const QFontPrivate *other);

    static void detachButKeepEngineData(QFont *font);

private:
    QFontPrivate &operator=(const QFontPrivate &) { return *this; }
};


class Q_GUI_EXPORT QFontCache : public QObject
{
public:
    // note: these static functions work on a per-thread basis
    static QFontCache *instance();
    static void cleanup();

    QFontCache();
    ~QFontCache();

    int id() const { return m_id; }

    void clear();

    struct Key {
        Key() : script(0), multi(0) { }
        Key(const QFontDef &d, uchar c, bool m = 0)
            : def(d), script(c), multi(m) { }

        QFontDef def;
        uchar script;
        uchar multi: 1;

        inline bool operator<(const Key &other) const
        {
            if (script != other.script) return script < other.script;
            if (multi != other.multi) return multi < other.multi;
            if (multi && def.fallBackFamilies.size() != other.def.fallBackFamilies.size())
                return def.fallBackFamilies.size() < other.def.fallBackFamilies.size();
            return def < other.def;
        }
        inline bool operator==(const Key &other) const
        {
            return script == other.script
                    && multi == other.multi
                    && (!multi || def.fallBackFamilies == other.def.fallBackFamilies)
                    && def == other.def;
        }
    };

    // QFontEngineData cache
    typedef QMap<QFontDef, QFontEngineData*> EngineDataCache;
    EngineDataCache engineDataCache;

    QFontEngineData *findEngineData(const QFontDef &def) const;
    void insertEngineData(const QFontDef &def, QFontEngineData *engineData);

    // QFontEngine cache
    struct Engine {
        Engine() : data(nullptr), timestamp(0), hits(0) { }
        Engine(QFontEngine *d) : data(d), timestamp(0), hits(0) { }

        QFontEngine *data;
        uint timestamp;
        uint hits;
    };

    typedef QMultiMap<Key,Engine> EngineCache;
    EngineCache engineCache;
    QHash<QFontEngine *, int> engineCacheCount;

    QFontEngine *findEngine(const Key &key);

    void updateHitCountAndTimeStamp(Engine &value);
    void insertEngine(const Key &key, QFontEngine *engine, bool insertMulti = false);

private:
    void increaseCost(uint cost);
    void decreaseCost(uint cost);
    void timerEvent(QTimerEvent *event) override;
    void decreaseCache();

    static const uint min_cost;
    uint total_cost, max_cost;
    uint current_timestamp;
    bool fast;
    int timer_id;
    const int m_id;
};

Q_GUI_EXPORT int qt_defaultDpiX();
Q_GUI_EXPORT int qt_defaultDpiY();
Q_GUI_EXPORT int qt_defaultDpi();

Q_GUI_EXPORT int qt_legacyToOpenTypeWeight(int weight);
Q_GUI_EXPORT int qt_openTypeToLegacyWeight(int weight);

QT_END_NAMESPACE

#endif // QFONT_P_H
