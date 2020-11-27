/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef QFONTDATABASE_P_H
#define QFONTDATABASE_P_H

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

#include <QtCore/qcache.h>

#include <QtGui/qfontdatabase.h>

QT_BEGIN_NAMESPACE

struct QtFontFallbacksCacheKey
{
    QString family;
    QFont::Style style;
    QFont::StyleHint styleHint;
    QChar::Script script;
};

inline bool operator==(const QtFontFallbacksCacheKey &lhs, const QtFontFallbacksCacheKey &rhs) noexcept
{
    return lhs.script == rhs.script &&
            lhs.styleHint == rhs.styleHint &&
            lhs.style == rhs.style &&
            lhs.family == rhs.family;
}

inline bool operator!=(const QtFontFallbacksCacheKey &lhs, const QtFontFallbacksCacheKey &rhs) noexcept
{
    return !operator==(lhs, rhs);
}

inline size_t qHash(const QtFontFallbacksCacheKey &key, size_t seed = 0) noexcept
{
    QtPrivate::QHashCombine hash;
    seed = hash(seed, key.family);
    seed = hash(seed, int(key.style));
    seed = hash(seed, int(key.styleHint));
    seed = hash(seed, int(key.script));
    return seed;
}

struct Q_GUI_EXPORT QtFontSize
{
    void *handle;
    unsigned short pixelSize : 16;
};

struct Q_GUI_EXPORT QtFontStyle
{
    struct Key
    {
        Key(const QString &styleString);

        Key()
            : style(QFont::StyleNormal)
            , weight(QFont::Normal)
            , stretch(0)
        {}

        Key(const Key &o)
            : style(o.style)
            , weight(o.weight)
            , stretch(o.stretch)
        {}

        uint style          : 2;
        uint weight         : 10;
        signed int stretch  : 12;

        bool operator==(const Key &other) const noexcept
        {
            return (style == other.style && weight == other.weight &&
                    (stretch == 0 || other.stretch == 0 || stretch == other.stretch));
        }

        bool operator!=(const Key &other) const noexcept
        {
            return !operator==(other);
        }
    };

    QtFontStyle(const Key &k)
        : key(k)
        , bitmapScalable(false)
        , smoothScalable(false)
        , count(0)
        , pixelSizes(nullptr)
    {
    }

    ~QtFontStyle();

    QtFontSize *pixelSize(unsigned short size, bool = false);

    Key key;
    bool bitmapScalable : 1;
    bool smoothScalable : 1;
    signed int count    : 30;
    QtFontSize *pixelSizes;
    QString styleName;
    bool antialiased;
};

struct Q_GUI_EXPORT QtFontFoundry
{
    QtFontFoundry(const QString &n)
        : name(n)
        , count(0)
        , styles(nullptr)
    {}

    ~QtFontFoundry()
    {
        while (count--)
            delete styles[count];
        free(styles);
    }

    QString name;
    int count;
    QtFontStyle **styles;
    QtFontStyle *style(const QtFontStyle::Key &, const QString & = QString(), bool = false);
};

struct Q_GUI_EXPORT QtFontFamily
{
    enum WritingSystemStatus {
        Unknown         = 0,
        Supported       = 1,
        UnsupportedFT  = 2,
        Unsupported     = UnsupportedFT
    };

    QtFontFamily(const QString &n)
        :
        populated(false),
        fixedPitch(false),
        name(n), count(0), foundries(nullptr)
    {
        memset(writingSystems, 0, sizeof(writingSystems));
    }
    ~QtFontFamily() {
        while (count--)
            delete foundries[count];
        free(foundries);
    }

    bool populated : 1;
    bool fixedPitch : 1;

    QString name;
    QStringList aliases;
    int count;
    QtFontFoundry **foundries;

    unsigned char writingSystems[QFontDatabase::WritingSystemsCount];

    bool matchesFamilyName(const QString &familyName) const;
    QtFontFoundry *foundry(const QString &f, bool = false);

    void ensurePopulated();
};

class Q_GUI_EXPORT QFontDatabasePrivate
{
public:
    QFontDatabasePrivate()
        : count(0)
        , families(nullptr)
        , fallbacksCache(64)
    { }

    ~QFontDatabasePrivate() {
        free();
    }

    enum FamilyRequestFlags {
        RequestFamily = 0,
        EnsureCreated,
        EnsurePopulated
    };

    QtFontFamily *family(const QString &f, FamilyRequestFlags flags = EnsurePopulated);
    void free() {
        while (count--)
            delete families[count];
        ::free(families);
        families = nullptr;
        count = 0;
        // don't clear the memory fonts!
    }

    int count;
    QtFontFamily **families;

    QCache<QtFontFallbacksCacheKey, QStringList> fallbacksCache;
    struct ApplicationFont {
        QString fileName;
        QByteArray data;

        struct Properties {
            QString familyName;
            QString styleName;
            int weight = 0;
            QFont::Style style = QFont::StyleNormal;
            int stretch = QFont::Unstretched;
        };

        QList<Properties> properties;
    };
    QList<ApplicationFont> applicationFonts;
    int addAppFont(const QByteArray &fontData, const QString &fileName);
    bool isApplicationFont(const QString &fileName);

    static QFontDatabasePrivate *instance();

    static void createDatabase();
    static void parseFontName(const QString &name, QString &foundry, QString &family);
    static QString resolveFontFamilyAlias(const QString &family);
    static QFontEngine *findFont(const QFontDef &request, int script /* QChar::Script */);
    static void load(const QFontPrivate *d, int script /* QChar::Script */);
    static QFontDatabasePrivate *ensureFontDatabase();

    void invalidate();
};
Q_DECLARE_TYPEINFO(QFontDatabasePrivate::ApplicationFont, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif // QFONTDATABASE_P_H
