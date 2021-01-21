/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QWINDOWSCLIPBOARD_H
#define QWINDOWSCLIPBOARD_H

#include "qwindowsinternalmimedata.h"

#include <qpa/qplatformclipboard.h>

QT_BEGIN_NAMESPACE

class QWindowsOleDataObject;

class QWindowsClipboardRetrievalMimeData : public QWindowsInternalMimeData {
public:

protected:
    IDataObject *retrieveDataObject() const override;
    void releaseDataObject(IDataObject *) const override;
};

class QWindowsClipboard : public QPlatformClipboard
{
    Q_DISABLE_COPY_MOVE(QWindowsClipboard)
public:
    QWindowsClipboard();
    ~QWindowsClipboard() override;
    void registerViewer(); // Call in initialization, when context is up.
    void cleanup();

    QMimeData *mimeData(QClipboard::Mode mode = QClipboard::Clipboard) override;
    void setMimeData(QMimeData *data, QClipboard::Mode mode = QClipboard::Clipboard) override;
    bool supportsMode(QClipboard::Mode mode) const override;
    bool ownsMode(QClipboard::Mode mode) const override;

    inline bool clipboardViewerWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *result);

    static QWindowsClipboard *instance() { return m_instance; }

    HWND clipboardViewer() const { return m_clipboardViewer; }

private:
    void clear();
    void releaseIData();
    inline void propagateClipboardMessage(UINT message, WPARAM wParam, LPARAM lParam) const;
    inline void unregisterViewer();
    inline bool ownsClipboard() const;

    static QWindowsClipboard *m_instance;

    QWindowsClipboardRetrievalMimeData m_retrievalData;
    QWindowsOleDataObject *m_data = nullptr;
    HWND m_clipboardViewer = nullptr;
    HWND m_nextClipboardViewer = nullptr;
    bool m_formatListenerRegistered = false;
};

QT_END_NAMESPACE

#endif // QWINDOWSCLIPBOARD_H
