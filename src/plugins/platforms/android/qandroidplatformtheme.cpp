// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "androidjnimain.h"
#include "androidjnimenu.h"
#include "qandroidplatformtheme.h"
#include "qandroidplatformmenubar.h"
#include "qandroidplatformmenu.h"
#include "qandroidplatformmenuitem.h"
#include "qandroidplatformdialoghelpers.h"
#include "qandroidplatformfiledialoghelper.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QJsonDocument>
#include <QVariant>

#include <private/qguiapplication_p.h>
#include <private/qhighdpiscaling_p.h>
#include <qandroidplatformintegration.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace {
    const int textStyle_bold = 1;
    const int textStyle_italic = 2;

    const int typeface_sans = 1;
    const int typeface_serif = 2;
    const int typeface_monospace = 3;
}

static int fontType(const QString &androidControl)
{
    if (androidControl == "defaultStyle"_L1)
        return QPlatformTheme::SystemFont;
    if (androidControl == "textViewStyle"_L1)
        return QPlatformTheme::LabelFont;
    else if (androidControl == "buttonStyle"_L1)
        return QPlatformTheme::PushButtonFont;
    else if (androidControl == "checkboxStyle"_L1)
        return QPlatformTheme::CheckBoxFont;
    else if (androidControl == "radioButtonStyle"_L1)
        return QPlatformTheme::RadioButtonFont;
    else if (androidControl == "simple_list_item_single_choice"_L1)
        return QPlatformTheme::ItemViewFont;
    else if (androidControl == "simple_spinner_dropdown_item"_L1)
        return QPlatformTheme::ComboMenuItemFont;
    else if (androidControl == "spinnerStyle"_L1)
        return QPlatformTheme::ComboLineEditFont;
    else if (androidControl == "simple_list_item"_L1)
        return QPlatformTheme::ListViewFont;
    return -1;
}

static int paletteType(const QString &androidControl)
{
    if (androidControl == "defaultStyle"_L1)
        return QPlatformTheme::SystemPalette;
    if (androidControl == "textViewStyle"_L1)
        return QPlatformTheme::LabelPalette;
    else if (androidControl == "buttonStyle"_L1)
        return QPlatformTheme::ButtonPalette;
    else if (androidControl == "checkboxStyle"_L1)
        return QPlatformTheme::CheckBoxPalette;
    else if (androidControl == "radioButtonStyle"_L1)
        return QPlatformTheme::RadioButtonPalette;
    else if (androidControl == "simple_list_item_single_choice"_L1)
        return QPlatformTheme::ItemViewPalette;
    else if (androidControl == "editTextStyle"_L1)
        return QPlatformTheme::TextLineEditPalette;
    else if (androidControl == "spinnerStyle"_L1)
        return QPlatformTheme::ComboBoxPalette;
    return -1;
}

static void setPaletteColor(const QVariantMap &object,
                                    QPalette &palette,
                                    QPalette::ColorRole role)
{
    // QPalette::Active -> ENABLED_FOCUSED_WINDOW_FOCUSED_STATE_SET
    palette.setColor(QPalette::Active,
                     role,
                     QRgb(object.value("ENABLED_FOCUSED_WINDOW_FOCUSED_STATE_SET"_L1).toInt()));

    // QPalette::Inactive -> ENABLED_STATE_SET
    palette.setColor(QPalette::Inactive,
                     role,
                     QRgb(object.value("ENABLED_STATE_SET"_L1).toInt()));

    // QPalette::Disabled -> EMPTY_STATE_SET
    palette.setColor(QPalette::Disabled,
                     role,
                     QRgb(object.value("EMPTY_STATE_SET"_L1).toInt()));

    palette.setColor(QPalette::Current, role, palette.color(QPalette::Active, role));

    if (role == QPalette::WindowText) {
        // QPalette::BrightText -> PRESSED
        // QPalette::Active -> PRESSED_ENABLED_FOCUSED_WINDOW_FOCUSED_STATE_SET
        palette.setColor(QPalette::Active,
                         QPalette::BrightText,
                         QRgb(object.value("PRESSED_ENABLED_FOCUSED_WINDOW_FOCUSED_STATE_SET"_L1).toInt()));

        // QPalette::Inactive -> PRESSED_ENABLED_STATE_SET
        palette.setColor(QPalette::Inactive,
                         QPalette::BrightText,
                         QRgb(object.value("PRESSED_ENABLED_STATE_SET"_L1).toInt()));

        // QPalette::Disabled -> PRESSED_STATE_SET
        palette.setColor(QPalette::Disabled,
                         QPalette::BrightText,
                         QRgb(object.value("PRESSED_STATE_SET"_L1).toInt()));

        palette.setColor(QPalette::Current, QPalette::BrightText, palette.color(QPalette::Active, QPalette::BrightText));

        // QPalette::HighlightedText -> SELECTED
        // QPalette::Active -> ENABLED_SELECTED_WINDOW_FOCUSED_STATE_SET
        palette.setColor(QPalette::Active,
                         QPalette::HighlightedText,
                         QRgb(object.value("ENABLED_SELECTED_WINDOW_FOCUSED_STATE_SET"_L1).toInt()));

        // QPalette::Inactive -> ENABLED_SELECTED_STATE_SET
        palette.setColor(QPalette::Inactive,
                         QPalette::HighlightedText,
                         QRgb(object.value("ENABLED_SELECTED_STATE_SET"_L1).toInt()));

        // QPalette::Disabled -> SELECTED_STATE_SET
        palette.setColor(QPalette::Disabled,
                         QPalette::HighlightedText,
                         QRgb(object.value("SELECTED_STATE_SET"_L1).toInt()));

        palette.setColor(QPalette::Current,
                         QPalette::HighlightedText,
                         palette.color(QPalette::Active, QPalette::HighlightedText));

        // Same colors for Text
        palette.setColor(QPalette::Active, QPalette::Text, palette.color(QPalette::Active, role));
        palette.setColor(QPalette::Inactive, QPalette::Text, palette.color(QPalette::Inactive, role));
        palette.setColor(QPalette::Disabled, QPalette::Text, palette.color(QPalette::Disabled, role));
        palette.setColor(QPalette::Current, QPalette::Text, palette.color(QPalette::Current, role));

        // And for ButtonText
        palette.setColor(QPalette::Active, QPalette::ButtonText, palette.color(QPalette::Active, role));
        palette.setColor(QPalette::Inactive, QPalette::ButtonText, palette.color(QPalette::Inactive, role));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, palette.color(QPalette::Disabled, role));
        palette.setColor(QPalette::Current, QPalette::ButtonText, palette.color(QPalette::Current, role));
    }
}

QJsonObject AndroidStyle::loadStyleData()
{
    QString stylePath(QLatin1StringView(qgetenv("ANDROID_STYLE_PATH")));
    const QLatin1Char slashChar('/');
    if (!stylePath.isEmpty() && !stylePath.endsWith(slashChar))
        stylePath += slashChar;

    if (QAndroidPlatformIntegration::colorScheme() == Qt::ColorScheme::Dark)
        stylePath += "darkUiMode/"_L1;

    Q_ASSERT(!stylePath.isEmpty());

    QString androidTheme = QLatin1StringView(qgetenv("QT_ANDROID_THEME"));
    if (!androidTheme.isEmpty() && !androidTheme.endsWith(slashChar))
        androidTheme += slashChar;

    if (!androidTheme.isEmpty() && QFileInfo::exists(stylePath + androidTheme + "style.json"_L1))
        stylePath += androidTheme;

    QFile f(stylePath + "style.json"_L1);
    if (!f.open(QIODevice::ReadOnly))
        return QJsonObject();

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(f.readAll(), &error);
    if (Q_UNLIKELY(document.isNull())) {
        qCritical() << error.errorString();
        return QJsonObject();
    }

    if (Q_UNLIKELY(!document.isObject())) {
        qCritical("Style.json does not contain a valid style.");
        return QJsonObject();
    }
    return document.object();
}

static void loadAndroidStyle(QPalette *defaultPalette, std::shared_ptr<AndroidStyle> &style)
{
    double pixelDensity = QHighDpiScaling::isActive() ? QtAndroid::pixelDensity() : 1.0;
    if (style) {
        style->m_standardPalette = QPalette();
        style->m_palettes.clear();
        style->m_fonts.clear();
        style->m_QWidgetsFonts.clear();
    } else {
        style = std::make_shared<AndroidStyle>();
    }

    style->m_styleData = AndroidStyle::loadStyleData();

    if (style->m_styleData.isEmpty())
        return;

    {
        QFont font("Droid Sans Mono"_L1, 14.0 * 100 / 72);
        style->m_fonts.insert(QPlatformTheme::FixedFont, font);
    }

    for (QJsonObject::const_iterator objectIterator = style->m_styleData.constBegin();
         objectIterator != style->m_styleData.constEnd();
         ++objectIterator) {
        QString key = objectIterator.key();
        QJsonValue value = objectIterator.value();
        if (!value.isObject()) {
            qWarning("Style.json structure is unrecognized.");
            continue;
        }
        QJsonObject item = value.toObject();
        QJsonObject::const_iterator attributeIterator = item.find("qtClass"_L1);
        QByteArray qtClassName;
        if (attributeIterator != item.constEnd()) {
            // The item has palette and font information for a specific Qt Class (e.g. QWidget, QPushButton, etc.)
            qtClassName = attributeIterator.value().toString().toLatin1();
        }
        const int ft = fontType(key);
        if (ft > -1 || !qtClassName.isEmpty()) {
            // Extract font information
            QFont font;

            // Font size (in pixels)
            attributeIterator = item.find("TextAppearance_textSize"_L1);
            if (attributeIterator != item.constEnd())
                font.setPixelSize(int(attributeIterator.value().toDouble() / pixelDensity));

            // Font style
            attributeIterator = item.find("TextAppearance_textStyle"_L1);
            if (attributeIterator != item.constEnd()) {
                const int style = int(attributeIterator.value().toDouble());
                font.setBold(style & textStyle_bold);
                font.setItalic(style & textStyle_italic);
            }

            // Font typeface
            attributeIterator = item.find("TextAppearance_typeface"_L1);
            if (attributeIterator != item.constEnd()) {
                QFont::StyleHint styleHint = QFont::AnyStyle;
                switch (int(attributeIterator.value().toDouble())) {
                case typeface_sans:
                    styleHint = QFont::SansSerif;
                    break;
                case typeface_serif:
                    styleHint = QFont::Serif;
                    break;
                case typeface_monospace:
                    styleHint = QFont::Monospace;
                    break;
                }
                font.setStyleHint(styleHint, QFont::PreferMatch);
            }
            if (!qtClassName.isEmpty())
                style->m_QWidgetsFonts.insert(qtClassName, font);

            if (ft > -1) {
                style->m_fonts.insert(ft, font);
                if (ft == QPlatformTheme::SystemFont)
                    QGuiApplication::setFont(font);
            }
            // Extract font information
        }

        const int pt = paletteType(key);
        if (pt > -1 || !qtClassName.isEmpty()) {
            // Extract palette information
            QPalette palette = *defaultPalette;

            attributeIterator = item.find("defaultTextColorPrimary"_L1);
            if (attributeIterator != item.constEnd())
                palette.setColor(QPalette::WindowText, QRgb(int(attributeIterator.value().toDouble())));

            attributeIterator = item.find("defaultBackgroundColor"_L1);
            if (attributeIterator != item.constEnd())
                palette.setColor(QPalette::Window, QRgb(int(attributeIterator.value().toDouble())));

            attributeIterator = item.find("TextAppearance_textColor"_L1);
            if (attributeIterator != item.constEnd())
                setPaletteColor(attributeIterator.value().toObject().toVariantMap(), palette, QPalette::WindowText);

            attributeIterator = item.find("TextAppearance_textColorLink"_L1);
            if (attributeIterator != item.constEnd())
                setPaletteColor(attributeIterator.value().toObject().toVariantMap(), palette, QPalette::Link);

            attributeIterator = item.find("TextAppearance_textColorHighlight"_L1);
            if (attributeIterator != item.constEnd())
                palette.setColor(QPalette::Highlight, QRgb(int(attributeIterator.value().toDouble())));

            if (pt == QPlatformTheme::SystemPalette)
                *defaultPalette = style->m_standardPalette = palette;

            if (pt > -1)
                style->m_palettes.insert(pt, palette);
            // Extract palette information
        }
    }
}

QAndroidPlatformTheme *QAndroidPlatformTheme::m_instance = nullptr;

QAndroidPlatformTheme *QAndroidPlatformTheme::instance(
    QAndroidPlatformNativeInterface *androidPlatformNativeInterface)
{
    if (androidPlatformNativeInterface && !m_instance) {
        m_instance = new QAndroidPlatformTheme(androidPlatformNativeInterface);
    }
    return m_instance;
}

QAndroidPlatformTheme::QAndroidPlatformTheme(QAndroidPlatformNativeInterface *androidPlatformNativeInterface)
{
    updateStyle();

    androidPlatformNativeInterface->m_androidStyle = m_androidStyleData;

    // default in case the style has not set a font
    m_systemFont = QFont("Roboto"_L1, 14.0 * 100 / 72); // keep default size the same after changing from 100 dpi to 72 dpi
}

QAndroidPlatformTheme::~QAndroidPlatformTheme()
{
    m_instance = nullptr;
}

void QAndroidPlatformTheme::updateColorScheme()
{
    updateStyle();
    QWindowSystemInterface::handleThemeChange();
}

void QAndroidPlatformTheme::updateStyle()
{
    QColor windowText = Qt::black;
    QColor background(229, 229, 229);
    QColor light = background.lighter(150);
    QColor mid(background.darker(130));
    QColor midLight = mid.lighter(110);
    QColor base(249, 249, 249);
    QColor disabledBase(background);
    QColor dark = background.darker(150);
    QColor darkDisabled = dark.darker(110);
    QColor text = Qt::black;
    QColor highlightedText = Qt::black;
    QColor disabledText = QColor(190, 190, 190);
    QColor button(241, 241, 241);
    QColor shadow(201, 201, 201);
    QColor highlight(148, 210, 231);
    QColor disabledShadow = shadow.lighter(150);

    if (colorScheme() == Qt::ColorScheme::Dark) {
        // Colors were prepared based on Theme.DeviceDefault.DayNight
        windowText = QColor(250, 250, 250);
        background = QColor(48, 48, 48);
        light = background.darker(150);
        mid = background.lighter(130);
        midLight = mid.darker(110);
        base = background;
        disabledBase = background;
        dark = background.darker(150);
        darkDisabled = dark.darker(110);
        text = QColor(250, 250, 250);
        highlightedText = QColor(250, 250, 250);
        disabledText = QColor(96, 96, 96);
        button = QColor(48, 48, 48);
        shadow = QColor(32, 32, 32);
        highlight = QColor(102, 178, 204);
        disabledShadow = shadow.darker(150);
    }

    m_defaultPalette = QPalette(windowText,background,light,dark,mid,text,base);
    m_defaultPalette.setBrush(QPalette::Midlight, midLight);
    m_defaultPalette.setBrush(QPalette::Button, button);
    m_defaultPalette.setBrush(QPalette::Shadow, shadow);
    m_defaultPalette.setBrush(QPalette::HighlightedText, highlightedText);

    m_defaultPalette.setBrush(QPalette::Disabled, QPalette::Text, disabledText);
    m_defaultPalette.setBrush(QPalette::Disabled, QPalette::WindowText, disabledText);
    m_defaultPalette.setBrush(QPalette::Disabled, QPalette::ButtonText, disabledText);
    m_defaultPalette.setBrush(QPalette::Disabled, QPalette::Base, disabledBase);
    m_defaultPalette.setBrush(QPalette::Disabled, QPalette::Dark, darkDisabled);
    m_defaultPalette.setBrush(QPalette::Disabled, QPalette::Shadow, disabledShadow);

    m_defaultPalette.setBrush(QPalette::Active, QPalette::Highlight, highlight);
    m_defaultPalette.setBrush(QPalette::Inactive, QPalette::Highlight, highlight);
    m_defaultPalette.setBrush(QPalette::Disabled, QPalette::Highlight, highlight.lighter(150));

    loadAndroidStyle(&m_defaultPalette, m_androidStyleData);
}

QPlatformMenuBar *QAndroidPlatformTheme::createPlatformMenuBar() const
{
    return new QAndroidPlatformMenuBar;
}

QPlatformMenu *QAndroidPlatformTheme::createPlatformMenu() const
{
    return new QAndroidPlatformMenu;
}

QPlatformMenuItem *QAndroidPlatformTheme::createPlatformMenuItem() const
{
    return new QAndroidPlatformMenuItem;
}

void QAndroidPlatformTheme::showPlatformMenuBar()
{
    QtAndroidMenu::openOptionsMenu();
}

Qt::ColorScheme QAndroidPlatformTheme::colorScheme() const
{
    return QAndroidPlatformIntegration::colorScheme();
}

static inline int paletteType(QPlatformTheme::Palette type)
{
    switch (type) {
    case QPlatformTheme::ToolButtonPalette:
    case QPlatformTheme::ButtonPalette:
        return QPlatformTheme::ButtonPalette;

    case QPlatformTheme::CheckBoxPalette:
        return QPlatformTheme::CheckBoxPalette;

    case QPlatformTheme::RadioButtonPalette:
        return QPlatformTheme::RadioButtonPalette;

    case QPlatformTheme::ComboBoxPalette:
        return QPlatformTheme::ComboBoxPalette;

    case QPlatformTheme::TextEditPalette:
    case QPlatformTheme::TextLineEditPalette:
        return QPlatformTheme::TextLineEditPalette;

    case QPlatformTheme::ItemViewPalette:
        return QPlatformTheme::ItemViewPalette;

    default:
        return QPlatformTheme::SystemPalette;
    }
}

const QPalette *QAndroidPlatformTheme::palette(Palette type) const
{
    if (m_androidStyleData) {
        auto it = m_androidStyleData->m_palettes.find(paletteType(type));
        if (it != m_androidStyleData->m_palettes.end())
            return &(it.value());
    }
    return &m_defaultPalette;
}

static inline int fontType(QPlatformTheme::Font type)
{
    switch (type) {
    case QPlatformTheme::LabelFont:
        return QPlatformTheme::SystemFont;

    case QPlatformTheme::ToolButtonFont:
        return QPlatformTheme::PushButtonFont;

    default:
        return type;
    }
}

const QFont *QAndroidPlatformTheme::font(Font type) const
{
    if (m_androidStyleData) {
        auto it = m_androidStyleData->m_fonts.find(fontType(type));
        if (it != m_androidStyleData->m_fonts.end())
            return &(it.value());
    }

    if (type == QPlatformTheme::SystemFont)
        return &m_systemFont;
    return 0;
}

QVariant QAndroidPlatformTheme::themeHint(ThemeHint hint) const
{
    switch (hint) {
    case StyleNames:
        if (qEnvironmentVariableIntValue("QT_USE_ANDROID_NATIVE_STYLE")
                && m_androidStyleData) {
            return QStringList("android"_L1);
        }
        return QStringList("Fusion"_L1);
    case DialogButtonBoxLayout:
        return QVariant(QPlatformDialogHelper::AndroidLayout);
    case MouseDoubleClickDistance:
    {
            int minimumDistance = qEnvironmentVariableIntValue("QT_ANDROID_MINIMUM_MOUSE_DOUBLE_CLICK_DISTANCE");
            int ret = minimumDistance;

            QAndroidPlatformIntegration *platformIntegration
                    = static_cast<QAndroidPlatformIntegration *>(QGuiApplicationPrivate::platformIntegration());
            QAndroidPlatformScreen *platformScreen = platformIntegration->screen();
            if (platformScreen != 0) {
                QScreen *screen = platformScreen->screen();
                qreal dotsPerInch = screen->physicalDotsPerInch();

                // Allow 15% of an inch between clicks when double clicking
                int distance = qRound(dotsPerInch * 0.15);
                if (distance > minimumDistance)
                    ret = distance;
            }

            if (ret > 0)
                return ret;

            Q_FALLTHROUGH();
    }
    default:
        return QPlatformTheme::themeHint(hint);
    }
}

QString QAndroidPlatformTheme::standardButtonText(int button) const
{
    switch (button) {
    case QPlatformDialogHelper::Yes:
        return QCoreApplication::translate("QAndroidPlatformTheme", "Yes");
    case QPlatformDialogHelper::YesToAll:
        return QCoreApplication::translate("QAndroidPlatformTheme", "Yes to All");
    case QPlatformDialogHelper::No:
        return QCoreApplication::translate("QAndroidPlatformTheme", "No");
    case QPlatformDialogHelper::NoToAll:
        return QCoreApplication::translate("QAndroidPlatformTheme", "No to All");
    }
    return QPlatformTheme::standardButtonText(button);
}

bool QAndroidPlatformTheme::usePlatformNativeDialog(QPlatformTheme::DialogType type) const
{
    if (type == MessageDialog)
        return qEnvironmentVariableIntValue("QT_USE_ANDROID_NATIVE_DIALOGS") == 1;
    if (type == FileDialog)
        return true;
    return false;
}

QPlatformDialogHelper *QAndroidPlatformTheme::createPlatformDialogHelper(QPlatformTheme::DialogType type) const
{
    switch (type) {
    case MessageDialog:
        return new QtAndroidDialogHelpers::QAndroidPlatformMessageDialogHelper;
    case FileDialog:
        return new QtAndroidFileDialogHelper::QAndroidPlatformFileDialogHelper;
    default:
        return 0;
    }
}

QT_END_NAMESPACE
