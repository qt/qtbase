/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QGENERICUNIXTHEMES_H
#define QGENERICUNIXTHEMES_H

#include <qpa/qplatformtheme.h>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtGui/QFont>

QT_BEGIN_HEADER

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

class QGenericUnixTheme : public QPlatformTheme
{
public:
    QGenericUnixTheme();

    static QPlatformTheme *createUnixTheme(const QString &name);
    static QStringList themeNames();

    virtual const QFont *font(Font type) const;
    virtual QVariant themeHint(ThemeHint hint) const;

    static QStringList xdgIconThemePaths();

    static const char *name;

private:
    const QFont m_systemFont;
};

#ifndef QT_NO_SETTINGS
class QKdeTheme : public QPlatformTheme
{
    QKdeTheme(const QString &kdeHome, int kdeVersion);
public:

    static QPlatformTheme *createKdeTheme();
    virtual QVariant themeHint(ThemeHint hint) const;

    virtual const QPalette *palette(Palette type = SystemPalette) const
        { return m_resources.palettes[type]; }

    virtual const QFont *font(Font type) const
        { return m_resources.fonts[type]; }

    static const char *name;

private:
    QString globalSettingsFile() const;
    void refresh();

    const QString m_kdeHome;
    const int m_kdeVersion;

    ResourceHelper m_resources;
    QString m_iconThemeName;
    QString m_iconFallbackThemeName;
    QStringList m_styleNames;
    int m_toolButtonStyle;
    int m_toolBarIconSize;
};
#endif // QT_NO_SETTINGS

class QGnomeTheme : public QPlatformTheme
{
public:
    QGnomeTheme();
    virtual QVariant themeHint(ThemeHint hint) const;
    virtual const QFont *font(Font type) const;

    static const char *name;

private:
    const QFont m_systemFont;
};

QPlatformTheme *qt_createUnixTheme();

QT_END_NAMESPACE

QT_END_HEADER

#endif // QGENERICUNIXTHEMES_H
