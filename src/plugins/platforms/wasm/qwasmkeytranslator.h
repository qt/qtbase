// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWASMKEYTRANSLATOR_H
#define QWASMKEYTRANSLATOR_H

#include <QtCore/qnamespace.h>
#include <QtCore/qtypes.h>

#include <optional>

QT_BEGIN_NAMESPACE

struct KeyEvent;

namespace QWasmKeyTranslator {
std::optional<Qt::Key> mapWebKeyTextToQtKey(const char *toFind);
}

QT_END_NAMESPACE
#endif // QWASMKEYTRANSLATOR_H
