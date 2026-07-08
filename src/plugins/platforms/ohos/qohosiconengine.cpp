// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosiconengine.h"

#include <qohosplugincore.h>

#include <QtGui/qfont.h>
#include <QtGui/private/qfonticonengine_p.h>

#include <QtCore/qhash.h>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoslogger_p.h>

#include <optional>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace {

std::optional<char32_t> tryGetOhosSymbolCodepoint(QtOhos::JsState &jsState, const char *ohosSymbolName)
{
    try {
        return jsState.eval<QNapi::Number>(
            "@ohos.resourceManager.getSysResourceManager().getSymbolByName(*)",
            {ohosSymbolName}).Uint32Value();
    } catch (const Napi::Error &error) {
        qOhosPrintfWarning(
            "%s: getSymbolByName('%s') failed: %s",
            Q_FUNC_INFO, ohosSymbolName, error.what());
        return std::nullopt;
    }
}

const QHash<QString, char32_t> &getThemeIconCodepoints()
{
    // Note: Qt uses string based icon names, see QAbstractFileIconProviderPrivate::getIconThemeIcon
    static constexpr struct {
        QLatin1StringView qtThemeIconName;
        const char *ohosSymbolName;
    } themeIconToOhosSymbolMapping[] = {
        {"computer"_L1, "computer"},
        {"user-desktop"_L1, "desktop"},
        {"user-trash"_L1, "trash"},
        {"network-workgroup"_L1, "dot_radiowaves_left_and_right"},
        {"drive-harddisk"_L1, "externaldrive"},
        {"folder"_L1, "folder"},
        {"text-x-generic"_L1, "doc"},
    };

    static const QHash<QString, char32_t> codepoints = QtOhos::evalInJsThread(
        [](QtOhos::JsState &jsState) {
            QHash<QString, char32_t> codepoints;
            for (const auto &entry : themeIconToOhosSymbolMapping) {
                const auto optCodepoint = tryGetOhosSymbolCodepoint(jsState, entry.ohosSymbolName);
                if (optCodepoint)
                    codepoints.insert(entry.qtThemeIconName, *optCodepoint);
            }
            return codepoints;
        },
        Q_FUNC_INFO);

    return codepoints;
}

QFont ohosSymbolFont()
{
    QFont font("HM Symbol"_L1);
    font.setStyleStrategy(QFont::NoFontMerging);
    return font;
}

std::optional<QString> tryGetGlyphForThemeIcon(const QString &iconName)
{
    const QHash<QString, char32_t> &codepoints = getThemeIconCodepoints();
    const auto codepointIt = codepoints.constFind(iconName);
    if (codepointIt == codepoints.cend())
        return std::nullopt;

    const char32_t ucs4 = *codepointIt;

    return QString::fromUcs4(&ucs4, 1);
}

class QOhosIconEngine : public QFontIconEngine
{
public:
    explicit QOhosIconEngine(const QString &iconName, const QString &glyphString);

    QIconEngine *clone() const override;
    QString key() const override;

protected:
    QString string() const override;
    glyph_t glyph() const override;

private:
    QString m_glyphString;
};

QOhosIconEngine::QOhosIconEngine(const QString &iconName, const QString &glyphString)
    : QFontIconEngine(iconName, ohosSymbolFont())
    , m_glyphString(glyphString)
{
}

QIconEngine *QOhosIconEngine::clone() const
{
    auto *constThis = const_cast<QOhosIconEngine *>(this);
    return new QOhosIconEngine(constThis->iconName(), m_glyphString);
}

QString QOhosIconEngine::key() const
{
    return "QOhosIconEngine"_L1;
}

QString QOhosIconEngine::string() const
{
    return m_glyphString;
}

glyph_t QOhosIconEngine::glyph() const
{
    return 0;
}

}

QIconEngine *tryCreateQOhosIconEngine(const QString &iconName)
{
    auto optGlyphString = tryGetGlyphForThemeIcon(iconName);
    if (!optGlyphString)
        return nullptr;

    return new QOhosIconEngine(iconName, *optGlyphString);
}

QT_END_NAMESPACE
