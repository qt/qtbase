/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QCORETEXTFONTDATABASE_H
#define QCORETEXTFONTDATABASE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qglobal.h>

#include <qpa/qplatformfontdatabase.h>
#include <qpa/qplatformtheme.h>
#include <private/qcore_mac_p.h>

#ifdef Q_OS_OSX
#include <ApplicationServices/ApplicationServices.h>
#else
#include <CoreText/CoreText.h>
#include <CoreGraphics/CoreGraphics.h>
#endif

Q_DECLARE_METATYPE(QCFType<CGFontRef>);
Q_DECLARE_METATYPE(QCFType<CFURLRef>);

QT_BEGIN_NAMESPACE

class QCoreTextFontDatabase : public QPlatformFontDatabase
{
public:
    QCoreTextFontDatabase();
    ~QCoreTextFontDatabase();
    void populateFontDatabase() Q_DECL_OVERRIDE;
    bool populateFamilyAliases() override;
    void populateFamily(const QString &familyName) Q_DECL_OVERRIDE;
    void invalidate() override;

    QStringList fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script) const Q_DECL_OVERRIDE;
    QStringList addApplicationFont(const QByteArray &fontData, const QString &fileName) Q_DECL_OVERRIDE;
    void releaseHandle(void *handle) Q_DECL_OVERRIDE;
    bool isPrivateFontFamily(const QString &family) const Q_DECL_OVERRIDE;
    QFont defaultFont() const Q_DECL_OVERRIDE;
    bool fontsAlwaysScalable() const Q_DECL_OVERRIDE;
    QList<int> standardSizes() const Q_DECL_OVERRIDE;

    // For iOS and OS X platform themes
    QFont *themeFont(QPlatformTheme::Font) const;
    const QHash<QPlatformTheme::Font, QFont *> &themeFonts() const;

protected:
    mutable QSet<CTFontDescriptorRef> m_systemFontDescriptors;

private:
    void populateFromDescriptor(CTFontDescriptorRef font, const QString &familyName = QString());

    mutable QString defaultFontName;

    mutable QHash<QPlatformTheme::Font, QFont *> m_themeFonts;
    bool m_hasPopulatedAliases;
};

// Split out into separate template class so that the compiler doesn't have
// to generate code for each override in QCoreTextFontDatabase for each T.

template <class T>
class QCoreTextFontDatabaseEngineFactory : public QCoreTextFontDatabase
{
public:
    QFontEngine *fontEngine(const QFontDef &fontDef, void *handle) override;
    QFontEngine *fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference) override;
};

QT_END_NAMESPACE

#endif // QCORETEXTFONTDATABASE_H
