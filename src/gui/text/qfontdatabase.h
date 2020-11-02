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

#ifndef QFONTDATABASE_H
#define QFONTDATABASE_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qwindowdefs.h>
#include <QtCore/qstring.h>
#include <QtGui/qfont.h>

QT_BEGIN_NAMESPACE


struct QFontDef;
class QFontEngine;

class Q_GUI_EXPORT QFontDatabase
{
    Q_GADGET
public:
    enum WritingSystem {
        Any,

        Latin,
        Greek,
        Cyrillic,
        Armenian,
        Hebrew,
        Arabic,
        Syriac,
        Thaana,
        Devanagari,
        Bengali,
        Gurmukhi,
        Gujarati,
        Oriya,
        Tamil,
        Telugu,
        Kannada,
        Malayalam,
        Sinhala,
        Thai,
        Lao,
        Tibetan,
        Myanmar,
        Georgian,
        Khmer,
        SimplifiedChinese,
        TraditionalChinese,
        Japanese,
        Korean,
        Vietnamese,

        Symbol,
        Other = Symbol,

        Ogham,
        Runic,
        Nko,

        WritingSystemsCount
    };
    Q_ENUM(WritingSystem)

    enum SystemFont {
        GeneralFont,
        FixedFont,
        TitleFont,
        SmallestReadableFont
    };
    Q_ENUM(SystemFont)

    static QList<int> standardSizes();

#if QT_DEPRECATED_SINCE(6, 0)
    QT_DEPRECATED_VERSION_X_6_0("Call the static functions instead") explicit QFontDatabase() = default;
#else
    QFontDatabase() = delete;
#endif

    static QList<WritingSystem> writingSystems();
    static QList<WritingSystem> writingSystems(const QString &family);

    static QStringList families(WritingSystem writingSystem = Any);
    static QStringList styles(const QString &family);
    static QList<int> pointSizes(const QString &family, const QString &style = QString());
    static QList<int> smoothSizes(const QString &family, const QString &style);
    static QString styleString(const QFont &font);
    static QString styleString(const QFontInfo &fontInfo);

    static QFont font(const QString &family, const QString &style, int pointSize);

    static bool isBitmapScalable(const QString &family, const QString &style = QString());
    static bool isSmoothlyScalable(const QString &family, const QString &style = QString());
    static bool isScalable(const QString &family, const QString &style = QString());
    static bool isFixedPitch(const QString &family, const QString &style = QString());

    static bool italic(const QString &family, const QString &style);
    static bool bold(const QString &family, const QString &style);
    static int weight(const QString &family, const QString &style);

    static bool hasFamily(const QString &family);
    static bool isPrivateFamily(const QString &family);

    static QString writingSystemName(WritingSystem writingSystem);
    static QString writingSystemSample(WritingSystem writingSystem);

    static int addApplicationFont(const QString &fileName);
    static int addApplicationFontFromData(const QByteArray &fontData);
    static QStringList applicationFontFamilies(int id);
    static bool removeApplicationFont(int id);
    static bool removeAllApplicationFonts();

    static QFont systemFont(SystemFont type);
};

QT_END_NAMESPACE

#endif // QFONTDATABASE_H
