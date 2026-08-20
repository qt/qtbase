// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "blockingtestdialogs.h"

#include "shellhelpers.h"

#include <QtCore/qfile.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qpoint.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qthread.h>

#include <algorithm>
#include <chrono>
#include <cstdio>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

namespace {

QRegularExpression caseInsensitivePattern(const QString &pattern)
{
    return QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
}

QRect parseUiTestBounds(const QString &bounds)
{
    static const QRegularExpression pattern(u"\\[(-?\\d+),(-?\\d+)\\]\\[(-?\\d+),(-?\\d+)\\]"_s);
    const QRegularExpressionMatch match = pattern.match(bounds);
    if (!match.hasMatch())
        return {};
    return QRect(
        QPoint(match.captured(1).toInt(), match.captured(2).toInt()),
        QPoint(match.captured(3).toInt(), match.captured(4).toInt()));
}

QJsonObject dumpScreenLayout(const Hdc &hdc)
{
    constexpr auto layoutPathOnDevice = "/data/local/tmp/harmonyostestrunner-layout.json"_L1;

    hdc.shell({ u"uitest"_s, u"dumpLayout"_s, u"-p"_s, layoutPathOnDevice });
    return QJsonDocument::fromJson(readDeviceFile(hdc, layoutPathOnDevice).toUtf8()).object();
}

void clickAt(const Hdc &hdc, QPoint point)
{
    hdc.shell({ u"uitest"_s, u"uiInput"_s, u"click"_s,
        QString::number(point.x()), QString::number(point.y()) });
}

} // namespace

BlockingTestDialogs::BlockingTestDialogs(const QString &testConfigPath, Hdc hdc)
    : m_hdc(std::move(hdc))
{
    loadDeclarations(testConfigPath);
}

bool BlockingTestDialogs::isEmpty() const
{
    return m_dialogs.isEmpty();
}

void BlockingTestDialogs::loadDeclarations(const QString &testConfigPath)
{
    if (testConfigPath.isEmpty())
        return;

    QFile configFile(testConfigPath);
    if (!configFile.open(QIODevice::ReadOnly)) {
        fprintf(stderr, "harmonyostestrunner: cannot read the test config %s\n",
            qPrintable(testConfigPath));
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(configFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        fprintf(stderr, "harmonyostestrunner: test config %s is not valid JSON: %s\n",
            qPrintable(testConfigPath), qPrintable(parseError.errorString()));
        return;
    }

    const QJsonArray declaredDialogs =
        document.object().value(u"blockingTestDialogs"_s).toArray();
    for (const QJsonValue &value : declaredDialogs) {
        const std::optional<Dialog> dialog = parseDialog(value.toObject());
        if (dialog.has_value())
            m_dialogs.append(*dialog);
    }
}

std::optional<BlockingTestDialogs::Dialog>
BlockingTestDialogs::parseDialog(const QJsonObject &declaration)
{
    Dialog dialog;
    dialog.name = declaration.value(u"name"_s).toString();
    dialog.prompt = caseInsensitivePattern(declaration.value(u"prompt"_s).toString());
    const QJsonArray buttons = declaration.value(u"buttons"_s).toArray();
    for (const QJsonValue &button : buttons)
        dialog.buttons.append(caseInsensitivePattern(button.toString()));
    dialog.position = declaration.value(u"position"_s).toString();

    if (dialog.prompt.pattern().isEmpty() || dialog.buttons.isEmpty()) {
        fprintf(stderr, "harmonyostestrunner: ignoring blocking test dialog '%s': "
            "prompt and button patterns are required\n", qPrintable(dialog.name));
        return std::nullopt;
    }
    return dialog;
}

void BlockingTestDialogs::collectTexts(const QJsonObject &node, QList<ScreenText> &texts)
{
    const QJsonObject attributes = node.value(u"attributes"_s).toObject();
    const QString text = attributes.value(u"text"_s).toString().trimmed();
    if (!text.isEmpty())
        texts.append({ text, parseUiTestBounds(attributes.value(u"bounds"_s).toString()) });

    const QJsonArray children = node.value(u"children"_s).toArray();
    for (const QJsonValue &child : children)
        collectTexts(child.toObject(), texts);
}

QList<BlockingTestDialogs::ScreenText> BlockingTestDialogs::dumpScreenTexts() const
{
    QList<ScreenText> texts;
    collectTexts(dumpScreenLayout(m_hdc), texts);
    return texts;
}

bool BlockingTestDialogs::anyTextMatches(
    const QList<ScreenText> &texts, const QRegularExpression &pattern)
{
    return std::any_of(texts.cbegin(), texts.cend(), [&pattern](const ScreenText &screenText) {
        return pattern.match(screenText.text).hasMatch();
    });
}

std::optional<BlockingTestDialogs::ScreenText>
BlockingTestDialogs::findButton(
    const QList<ScreenText> &texts, const QRegularExpression &pattern,
    const QString &position)
{
    QList<ScreenText> candidates;
    for (const ScreenText &screenText : texts) {
        const QRegularExpressionMatch match = pattern.match(screenText.text);
        if (match.hasMatch() && match.capturedLength() == screenText.text.size()
            && !screenText.bounds.isNull())
            candidates.append(screenText);
    }
    if (candidates.isEmpty())
        return std::nullopt;

    auto byHorizontalCentre = [](const ScreenText &left, const ScreenText &right) {
        return left.bounds.center().x() < right.bounds.center().x();
    };
    if (position == u"leftmost"_s)
        return *std::min_element(candidates.cbegin(), candidates.cend(), byHorizontalCentre);
    return *std::max_element(candidates.cbegin(), candidates.cend(), byHorizontalCentre);
}

bool BlockingTestDialogs::pressButtons(const Dialog &dialog, const QList<ScreenText> &texts) const
{
    constexpr auto delayBetweenButtons = 700ms;

    bool pressedAnything = false;
    for (const QRegularExpression &buttonPattern : dialog.buttons) {
        const std::optional<ScreenText> button = findButton(texts, buttonPattern, dialog.position);
        if (!button.has_value())
            continue;

        const QPoint centre = button->bounds.center();
        fprintf(stderr, "harmonyostestrunner: answering blocking test dialog '%s': pressing "
            "\"%s\" at %d,%d\n", qPrintable(dialog.name), qPrintable(button->text),
            centre.x(), centre.y());
        clickAt(m_hdc, centre);
        pressedAnything = true;
        QThread::sleep(delayBetweenButtons);
    }
    return pressedAnything;
}

void BlockingTestDialogs::reportUnmatchedDialog(const QList<ScreenText> &texts)
{
    constexpr int maximumTextsInReportedDialog = 8;

    if (texts.size() > maximumTextsInReportedDialog)
        return;

    static const QRegularExpression declineButton =
        caseInsensitivePattern(u"^(cancel|deny|not now|later)$"_s);
    if (!anyTextMatches(texts, declineButton))
        return;

    QStringList onScreen;
    for (const ScreenText &screenText : texts)
        onScreen << screenText.text;

    const QString joined = onScreen.join(u" | "_s);
    if (joined == m_lastReportedUnknownDialog)
        return;

    m_lastReportedUnknownDialog = joined;
    fprintf(stderr, "harmonyostestrunner: dialog on screen matches no declaration, leaving it "
        "alone: %s\n", qPrintable(joined));
}

bool BlockingTestDialogs::answerVisibleDialog()
{
    if (m_dialogs.isEmpty())
        return false;

    const QList<ScreenText> texts = dumpScreenTexts();
    if (texts.isEmpty())
        return false;

    for (const Dialog &dialog : m_dialogs) {
        if (!anyTextMatches(texts, dialog.prompt))
            continue;

        const bool pressedAnything = pressButtons(dialog, texts);
        if (!pressedAnything) {
            fprintf(stderr, "harmonyostestrunner: blocking test dialog '%s' is on screen but none "
                "of its buttons matched the declaration\n", qPrintable(dialog.name));
        }
        return pressedAnything;
    }

    reportUnmatchedDialog(texts);
    return false;
}

QT_END_NAMESPACE
