// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <qpa/qplatformclipboard.h>
#include <qxcbobject.h>
#include <xcb/xcb.h>
#include <xcb/xfixes.h>

#include <QtCore/qbasictimer.h>
#include <QtCore/qobject.h>
#include <QtCore/qmap.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_CLIPBOARD

class QXcbConnection;
class QXcbScreen;
class QXcbClipboard;
class QXcbClipboardMime;

class QXcbClipboardTransaction : public QObject
{
    Q_OBJECT
public:
    QXcbClipboardTransaction(QXcbClipboard *clipboard, xcb_window_t w, xcb_atom_t p,
                           QByteArray d, xcb_atom_t t, int f);
    ~QXcbClipboardTransaction();

    bool updateIncrementalProperty(const xcb_property_notify_event_t *event);

protected:
    void timerEvent(QTimerEvent *ev) override;

private:
    QXcbClipboard *m_clipboard;
    xcb_window_t m_window;
    xcb_atom_t m_property;
    QByteArray m_data;
    xcb_atom_t m_target;
    uint8_t m_format;
    uint m_offset = 0;
    QBasicTimer m_abortTimer;
};

class QXcbClipboard : public QXcbObject, public QPlatformClipboard
{
public:
    QXcbClipboard(QXcbConnection *connection);
    ~QXcbClipboard();

    QMimeData *mimeData(QClipboard::Mode mode) override;
    void setMimeData(QMimeData *data, QClipboard::Mode mode) override;

    bool supportsMode(QClipboard::Mode mode) const override;
    bool ownsMode(QClipboard::Mode mode) const override;

    QXcbScreen *screen() const;

    xcb_window_t requestor() const;
    void setRequestor(xcb_window_t window);

    void handleSelectionRequest(xcb_selection_request_event_t *event);
    void handleSelectionClearRequest(xcb_selection_clear_event_t *event);
    void handleXFixesSelectionRequest(xcb_xfixes_selection_notify_event_t *event);

    bool clipboardReadProperty(xcb_window_t win, xcb_atom_t property, bool deleteProperty, QByteArray *buffer, int *size, xcb_atom_t *type, int *format);
    std::optional<QByteArray> clipboardReadIncrementalProperty(xcb_window_t win, xcb_atom_t property, int nbytes, bool nullterm);

    std::optional<QByteArray> getDataInFormat(xcb_atom_t modeAtom, xcb_atom_t fmtatom);

    bool handlePropertyNotify(const xcb_generic_event_t *event);

    std::optional<QByteArray> getSelection(xcb_atom_t selection, xcb_atom_t target, xcb_atom_t property, xcb_timestamp_t t = 0);

    int increment() const { return m_maxPropertyRequestDataBytes; }
    int clipboardTimeout() const { return clipboard_timeout; }

    void removeTransaction(xcb_window_t window) { m_transactions.remove(window); }

private:
    xcb_generic_event_t *waitForClipboardEvent(xcb_window_t window, int type, bool checkManager = false);

    xcb_atom_t sendTargetsSelection(QMimeData *d, xcb_window_t window, xcb_atom_t property);
    xcb_atom_t sendSelection(QMimeData *d, xcb_atom_t target, xcb_window_t window, xcb_atom_t property);

    xcb_atom_t atomForMode(QClipboard::Mode mode) const;
    QClipboard::Mode modeForAtom(xcb_atom_t atom) const;

    // Selection and Clipboard
    QScopedPointer<QXcbClipboardMime> m_xClipboard[2];
    QMimeData *m_clientClipboard[2];
    xcb_timestamp_t m_timestamp[2];

    xcb_window_t m_requestor = XCB_NONE;

    static const int clipboard_timeout;

    int m_maxPropertyRequestDataBytes = 0;
    bool m_clipboard_closing = false;
    xcb_timestamp_t m_incr_receive_time = 0;

    using TransactionMap = QMap<xcb_window_t, QXcbClipboardTransaction *>;
    TransactionMap m_transactions;
};

#endif // QT_NO_CLIPBOARD

QT_END_NAMESPACE
