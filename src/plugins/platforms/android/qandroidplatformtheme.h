// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDPLATFORMTHEME_H
#define QANDROIDPLATFORMTHEME_H

#include <qpa/qplatformtheme.h>
#include <QtGui/qfont.h>
#include <QtGui/qpalette.h>
#include <QtCore/qhash.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qloggingcategory.h>
#include <QJsonObject>

#include <memory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaMenus)

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
    ~QAndroidPlatformTheme();
    void updateColorScheme();
    void updateStyle();
    QPlatformMenuBar *createPlatformMenuBar() const override;
    QPlatformMenu *createPlatformMenu() const override;
    QPlatformMenuItem *createPlatformMenuItem() const override;
    void showPlatformMenuBar() override;
    Qt::ColorScheme colorScheme() const override;
    void requestColorScheme(Qt::ColorScheme scheme) override;

    const QPalette *palette(Palette type = SystemPalette) const override;
    const QFont *font(Font type = SystemFont) const override;
    QIconEngine *createIconEngine(const QString &iconName) const override;
    QVariant themeHint(ThemeHint hint) const override;
    QString standardButtonText(int button) const override;
    bool usePlatformNativeDialog(DialogType type) const override;
    QPlatformDialogHelper *createPlatformDialogHelper(DialogType type) const override;

    static QAndroidPlatformTheme *instance(
                    QAndroidPlatformNativeInterface * androidPlatformNativeInterface = nullptr);

private:
    QAndroidPlatformTheme(QAndroidPlatformNativeInterface * androidPlatformNativeInterface);
    static QAndroidPlatformTheme * m_instance;
    std::shared_ptr<AndroidStyle> m_androidStyleData;
    QPalette m_defaultPalette;
    QFont m_systemFont;
    Qt::ColorScheme m_colorSchemeOverride = Qt::ColorScheme::Unknown;
};

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMTHEME_H
