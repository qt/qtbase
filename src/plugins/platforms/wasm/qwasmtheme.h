// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMTHEME_H
#define QWASMTHEME_H

#include <qpa/qplatformtheme.h>
#include <QtGui/QFont>

QT_BEGIN_NAMESPACE

class QWasmEventTranslator;
class QWasmFontDatabase;
class QWasmWindow;
class QWasmEventDispatcher;
class QWasmScreen;
class QWasmCompositor;
class QWasmBackingStore;

class QWasmTheme : public QPlatformTheme
{
public:
    QWasmTheme();
    ~QWasmTheme();

    QVariant themeHint(ThemeHint hint) const override;
    const QFont *font(Font type) const override;
    QFont *fixedFont = nullptr;
};

QT_END_NAMESPACE

#endif // QWASMTHEME_H
