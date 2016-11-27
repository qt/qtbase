/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
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

#ifndef QANDROIDPLATFORMTHEME_H
#define QANDROIDPLATFORMTHEME_H

#include <qpa/qplatformtheme.h>
#include <QtGui/qfont.h>
#include <QtGui/qpalette.h>

#include <QJsonObject>

#include <memory>

QT_BEGIN_NAMESPACE

struct AndroidStyle
{
    static QJsonObject loadStyleData();
    QJsonObject m_styleData;
    QPalette m_standardPalette;
    QHash<int, QPalette> m_palettes;
    QHash<int, QFont> m_fonts;
    QHash<QByteArray, QFont> m_QWidgetsFonts;
};

class QAndroidPlatformNativeInterface;
class QAndroidPlatformTheme: public QPlatformTheme
{
public:
    QAndroidPlatformTheme(QAndroidPlatformNativeInterface * androidPlatformNativeInterface);
    QPlatformMenuBar *createPlatformMenuBar() const override;
    QPlatformMenu *createPlatformMenu() const override;
    QPlatformMenuItem *createPlatformMenuItem() const override;
    void showPlatformMenuBar() override;
    const QPalette *palette(Palette type = SystemPalette) const override;
    const QFont *font(Font type = SystemFont) const override;
    QVariant themeHint(ThemeHint hint) const override;
    QString standardButtonText(int button) const Q_DECL_OVERRIDE;
    bool usePlatformNativeDialog(DialogType type) const override;
    QPlatformDialogHelper *createPlatformDialogHelper(DialogType type) const override;


private:
    std::shared_ptr<AndroidStyle> m_androidStyleData;
    QPalette m_defaultPalette;
    QFont m_systemFont;
};

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMTHEME_H
