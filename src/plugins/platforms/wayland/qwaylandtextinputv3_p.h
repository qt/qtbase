// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWAYLANDTEXTINPUTV3_P_H
#define QWAYLANDTEXTINPUTV3_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qwaylandtextinputinterface_p.h"
#include <QtWaylandClient/private/qwayland-text-input-unstable-v3.h>
#include <QLoggingCategory>
#include <QList>

struct wl_callback;
struct wl_callback_listener;

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcQpaWaylandTextInput)

namespace QtWaylandClient {

class QWaylandDisplay;

class QWaylandTextInputv3 : public QtWayland::zwp_text_input_v3, public QWaylandTextInputInterface
{
public:
    QWaylandTextInputv3(QWaylandDisplay *display, struct ::zwp_text_input_v3 *text_input);
    ~QWaylandTextInputv3() override;

    void reset() override;
    void commit() override;
    void updateState(Qt::InputMethodQueries queries, uint32_t flags) override;
    // TODO: not supported yet
    void setCursorInsidePreedit(int cursor) override;

    bool isInputPanelVisible() const override;
    QRectF keyboardRect() const override;

    QLocale locale() const override;
    Qt::LayoutDirection inputDirection() const override;

    void showInputPanel() override;
    void hideInputPanel() override;

    // doing nothing in zwp_text_input_v3.
    // enter() and leave() takes the role to enable/disable the surface
    void enableSurface(::wl_surface *) override {};
    void disableSurface(::wl_surface *) override {};

protected:
    void zwp_text_input_v3_enter(struct ::wl_surface *surface) override;
    void zwp_text_input_v3_leave(struct ::wl_surface *surface) override;
    void zwp_text_input_v3_preedit_string(const QString &text, int32_t cursor_begin, int32_t cursor_end) override;
    void zwp_text_input_v3_commit_string(const QString &text) override;
    void zwp_text_input_v3_delete_surrounding_text(uint32_t before_length, uint32_t after_length) override;
    void zwp_text_input_v3_done(uint32_t serial) override;
    void zwp_text_input_v3_language(const QString &language) override;

    void zwp_text_input_v3_action(uint32_t action, uint32_t serial) override;
    void zwp_text_input_v3_preedit_hint(uint32_t begin, uint32_t end, uint32_t hint) override;

private:
    Q_DISABLE_COPY(QWaylandTextInputv3)

    ::wl_surface *m_surface = nullptr; // ### Here for debugging purposes

    struct StyleHint {
        uint32_t begin = 0;
        uint32_t end = 0;
        preedit_hint hint;
        bool operator==(const StyleHint &other) const {
            return begin == other.begin && end == other.end && hint == other.hint;
        }
    };

    struct PreeditInfo {
        QString text;
        int cursorBegin = 0;
        int cursorEnd = 0;

        QList<QWaylandTextInputv3::StyleHint> styleHints;

        void clear() {
            text.clear();
            styleHints.clear();
            cursorBegin = 0;
            cursorEnd = 0;
        }
        friend bool operator==(const PreeditInfo& lhs, const PreeditInfo& rhs) {
            return (lhs.text == rhs.text)
                    && (lhs.cursorBegin == rhs.cursorBegin)
                    && (lhs.cursorEnd == rhs.cursorEnd)
                    && lhs.styleHints == rhs.styleHints;
        }
    };

    PreeditInfo m_pendingPreeditString;
    PreeditInfo m_currentPreeditString;
    QString m_pendingCommitString;
    uint m_pendingDeleteBeforeText = 0; // byte length
    uint m_pendingDeleteAfterText = 0;  // byte length

    QString m_surroundingText;
    int m_cursor = 0; // cursor position in QString
    int m_cursorPos = 0; // cursor position in wayland index
    int m_anchorPos = 0; // anchor position in wayland index
    uint32_t m_contentHint = 0;
    uint32_t m_contentPurpose = 0;
    QRect m_cursorRect;

    uint m_currentSerial = 0;

    bool m_condReselection = false;
    QLocale m_locale;

};

}

QT_END_NAMESPACE

#endif // QWAYLANDTEXTINPUTV3_P_H
