// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWasmClipboard_H
#define QWasmClipboard_H

#include <QObject>

#include <qpa/qplatformclipboard.h>
#include <QMimeData>

#include <emscripten/bind.h>
#include <emscripten/val.h>

QT_BEGIN_NAMESPACE

struct KeyEvent;

class QWasmClipboard : public QObject, public QPlatformClipboard
{
public:
    enum class ProcessKeyboardResult {
        Ignored,
        NativeClipboardEventNeeded,
        NativeClipboardEventAndCopiedDataNeeded,
    };

    QWasmClipboard();
    virtual ~QWasmClipboard();

    // QPlatformClipboard methods.
    QMimeData* mimeData(QClipboard::Mode mode = QClipboard::Clipboard) override;
    void setMimeData(QMimeData* data, QClipboard::Mode mode = QClipboard::Clipboard) override;
    bool supportsMode(QClipboard::Mode mode) const override;
    bool ownsMode(QClipboard::Mode mode) const override;

    ProcessKeyboardResult processKeyboard(const KeyEvent &event);
    static void installEventHandlers(const emscripten::val &target);
    bool hasClipboardApi();

private:
    void initClipboardPermissions();
    void writeToClipboardApi();
    void writeToClipboard();

    bool m_hasClipboardApi = false;
};

QT_END_NAMESPACE

#endif // QWASMCLIPBOARD_H
