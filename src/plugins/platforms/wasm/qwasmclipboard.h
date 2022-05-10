// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWasmClipboard_H
#define QWasmClipboard_H

#include <QObject>

#include <qpa/qplatformclipboard.h>
#include <QMimeData>

#include <emscripten/bind.h>
#include <emscripten/val.h>

class QWasmClipboard : public QObject, public QPlatformClipboard
{
public:
    QWasmClipboard();
    virtual ~QWasmClipboard();

    // QPlatformClipboard methods.
    QMimeData* mimeData(QClipboard::Mode mode = QClipboard::Clipboard) override;
    void setMimeData(QMimeData* data, QClipboard::Mode mode = QClipboard::Clipboard) override;
    bool supportsMode(QClipboard::Mode mode) const override;
    bool ownsMode(QClipboard::Mode mode) const override;

    static void qWasmClipboardPaste(QMimeData *mData);
    void initClipboardPermissions();
    void installEventHandlers(const emscripten::val &canvas);
    bool hasClipboardApi;
    bool hasPermissionsApi;
    void writeToClipboardApi();
    void writeToClipboard(const QMimeData *data);
    bool isPaste;
    bool m_isListener;
    bool isSafari;
};

#endif // QWASMCLIPBOARD_H
