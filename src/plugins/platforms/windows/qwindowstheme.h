/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINDOWSTHEME_H
#define QWINDOWSTHEME_H

#include <QtGui/QPlatformTheme>
#include <QtGui/QColor>

#include "qtwindows_additional.h"

QT_BEGIN_NAMESPACE

class QWindow;

class QWindowsTheme : public QPlatformTheme
{
public:
    QWindowsTheme();
    ~QWindowsTheme();

    static QWindowsTheme *instance() { return m_instance; }

    virtual bool usePlatformNativeDialog(DialogType type) const;
    virtual QPlatformDialogHelper *createPlatformDialogHelper(DialogType type) const;
    virtual QVariant themeHint(ThemeHint) const;
    virtual const QPalette *palette(Palette type = SystemPalette) const
        { return m_palettes[type]; }
    virtual const QFont *font(Font type = SystemFont) const
        { return m_fonts[type]; }

    void windowsThemeChanged(QWindow *window);

    static const char *name;

private:
    void refresh() { refreshPalettes(); refreshFonts(); }
    void clearPalettes();
    void refreshPalettes();
    void clearFonts();
    void refreshFonts();

    static QWindowsTheme *m_instance;
    QPalette *m_palettes[NPalettes];
    QFont *m_fonts[NFonts];
};

static inline COLORREF qColorToCOLORREF(const QColor &color)
{ return RGB(color.red(), color.green(), color.blue()); }

static inline QColor COLORREFToQColor(COLORREF cr)
{ return QColor(GetRValue(cr), GetGValue(cr), GetBValue(cr)); }

QT_END_NAMESPACE

#endif // QWINDOWSTHEME_H
