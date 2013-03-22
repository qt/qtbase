/***************************************************************************
**
** Copyright (C) 2012 Research In Motion
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

#include "qqnxtheme.h"

#include "qqnxfiledialoghelper.h"
#include "qqnxsystemsettings.h"
#include "qqnxintegration.h"

QT_BEGIN_NAMESPACE

QQnxTheme::QQnxTheme(const QQnxIntegration *integration) : m_integration(integration)
{
}

QQnxTheme::~QQnxTheme()
{
    qDeleteAll(m_fonts);
}

bool QQnxTheme::usePlatformNativeDialog(DialogType type) const
{
#if defined(Q_OS_BLACKBERRY_TABLET)
    if (type == QPlatformTheme::FileDialog)
        return true;
#endif
#if !defined(QT_NO_COLORDIALOG)
    if (type == QPlatformTheme::ColorDialog)
        return false;
#endif
#if !defined(QT_NO_FONTDIALOG)
    if (type == QPlatformTheme::FontDialog)
        return false;
#endif
    return false;
}

QPlatformDialogHelper *QQnxTheme::createPlatformDialogHelper(DialogType type) const
{
    switch (type) {
#if defined(Q_OS_BLACKBERRY_TABLET)
    case QPlatformTheme::FileDialog:
        return new QQnxFileDialogHelper(m_integration);
#endif
#if !defined(QT_NO_COLORDIALOG)
    case QPlatformTheme::ColorDialog:
#endif
#if !defined(QT_NO_FONTDIALOG)
    case QPlatformTheme::FontDialog:
#endif
    default:
        return 0;
    }
}

const QFont *QQnxTheme::font(Font type) const
{
    QPlatformFontDatabase *fontDatabase = m_integration->fontDatabase();

    if (fontDatabase && m_fonts.isEmpty())
        m_fonts = qt_qnx_createRoleFonts(fontDatabase);
    return m_fonts.value(type, 0);
}

QT_END_NAMESPACE
