// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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

class QWasmDeadKeySupport
{
public:
    explicit QWasmDeadKeySupport();
    ~QWasmDeadKeySupport();

    void applyDeadKeyTranslations(KeyEvent *event);

private:
    Qt::Key m_activeDeadKey = Qt::Key_unknown;
    Qt::Key m_keyModifiedByDeadKeyOnPress = Qt::Key_unknown;
};

QT_END_NAMESPACE
#endif // QWASMKEYTRANSLATOR_H
