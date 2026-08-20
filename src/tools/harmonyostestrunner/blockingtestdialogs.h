// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef BLOCKINGTESTDIALOGS_H
#define BLOCKINGTESTDIALOGS_H

#include "hdc.h"

#include <QtCore/qjsonobject.h>
#include <QtCore/qlist.h>
#include <QtCore/qrect.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qstring.h>

#include <optional>

QT_BEGIN_NAMESPACE

class BlockingTestDialogs
{
public:
    BlockingTestDialogs(const QString &testConfigPath, Hdc hdc);

    bool isEmpty() const;

    bool answerVisibleDialog();

private:
    struct Dialog
    {
        QString name;
        QRegularExpression prompt;
        QList<QRegularExpression> buttons;
        QString position;
    };

    struct ScreenText
    {
        QString text;
        QRect bounds;
    };

    void loadDeclarations(const QString &testConfigPath);
    static std::optional<Dialog> parseDialog(const QJsonObject &declaration);

    QList<ScreenText> dumpScreenTexts() const;
    static void collectTexts(const QJsonObject &node, QList<ScreenText> &texts);

    static bool anyTextMatches(
        const QList<ScreenText> &texts, const QRegularExpression &pattern);
    static std::optional<ScreenText> findButton(
        const QList<ScreenText> &texts, const QRegularExpression &pattern,
        const QString &position);
    bool pressButtons(const Dialog &dialog, const QList<ScreenText> &texts) const;
    void reportUnmatchedDialog(const QList<ScreenText> &texts);

    QList<Dialog> m_dialogs;
    Hdc m_hdc;
    QString m_lastReportedUnknownDialog;
};

QT_END_NAMESPACE

#endif // BLOCKINGTESTDIALOGS_H
