/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QGENERICUNIXTHEMES_H
#define QGENERICUNIXTHEMES_H

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

#include <qpa/qplatformtheme.h>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtGui/QFont>

QT_BEGIN_NAMESPACE

class ResourceHelper
{
public:
    ResourceHelper();
    ~ResourceHelper() { clear(); }

    void clear();

    QPalette *palettes[QPlatformTheme::NPalettes];
    QFont *fonts[QPlatformTheme::NFonts];
};

class QGenericUnixThemePrivate;

class QGenericUnixTheme : public QPlatformTheme
{
    Q_DECLARE_PRIVATE(QGenericUnixTheme)
public:
    QGenericUnixTheme();

    static QPlatformTheme *createUnixTheme(const QString &name);
    static QStringList themeNames();

    const QFont *font(Font type) const override;
    QVariant themeHint(ThemeHint hint) const override;

    static QStringList xdgIconThemePaths();
    static QStringList iconFallbackPaths();
#ifndef QT_NO_DBUS
    QPlatformMenuBar *createPlatformMenuBar() const override;
#endif
#if !defined(QT_NO_DBUS) && !defined(QT_NO_SYSTEMTRAYICON)
    QPlatformSystemTrayIcon *createPlatformSystemTrayIcon() const override;
#endif

    static const char *name;
};

#if QT_CONFIG(settings)
class QKdeThemePrivate;

class QKdeTheme : public QPlatformTheme
{
    Q_DECLARE_PRIVATE(QKdeTheme)
public:
    QKdeTheme(const QStringList& kdeDirs, int kdeVersion);

    static QPlatformTheme *createKdeTheme();
    QVariant themeHint(ThemeHint hint) const override;

    QIcon fileIcon(const QFileInfo &fileInfo,
                   QPlatformTheme::IconOptions iconOptions = { }) const override;

    const QPalette *palette(Palette type = SystemPalette) const override;

    const QFont *font(Font type) const override;
#ifndef QT_NO_DBUS
    QPlatformMenuBar *createPlatformMenuBar() const override;
#endif
#if !defined(QT_NO_DBUS) && !defined(QT_NO_SYSTEMTRAYICON)
    QPlatformSystemTrayIcon *createPlatformSystemTrayIcon() const override;
#endif

    static const char *name;
};
#endif // settings

class QGnomeThemePrivate;

class QGnomeTheme : public QPlatformTheme
{
    Q_DECLARE_PRIVATE(QGnomeTheme)
public:
    QGnomeTheme();
    QVariant themeHint(ThemeHint hint) const override;
    QIcon fileIcon(const QFileInfo &fileInfo,
                   QPlatformTheme::IconOptions = { }) const override;
    const QFont *font(Font type) const override;
    QString standardButtonText(int button) const override;

    virtual QString gtkFontName() const;
#ifndef QT_NO_DBUS
    QPlatformMenuBar *createPlatformMenuBar() const override;
#endif
#if !defined(QT_NO_DBUS) && !defined(QT_NO_SYSTEMTRAYICON)
    QPlatformSystemTrayIcon *createPlatformSystemTrayIcon() const override;
#endif

    static const char *name;
};

QPlatformTheme *qt_createUnixTheme();

QT_END_NAMESPACE

#endif // QGENERICUNIXTHEMES_H
