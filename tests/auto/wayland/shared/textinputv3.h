// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MOCKCOMPOSITOR_TEXTINPUTV3_H
#define MOCKCOMPOSITOR_TEXTINPUTV3_H

#include "coreprotocol.h"
#include <qwayland-server-text-input-unstable-v3.h>

namespace MockCompositor {

class TextInputV3 : public QObject, public QtWaylandServer::zwp_text_input_v3
{
    Q_OBJECT
public:
    // Requests are double-buffered: they fill the pending state, which becomes the
    // committed state on the next commit request. enable and disable reset it.
    struct State {
        bool enabled = false;
        uint contentHint = content_hint_none;
        uint contentPurpose = content_purpose_normal;
    };

    void sendEnter(Surface *surface);
    void sendLeave(Surface *surface);

    State m_pending;
    State m_committed;

protected:
    void zwp_text_input_v3_destroy(Resource *resource) override;
    void zwp_text_input_v3_enable(Resource *resource) override;
    void zwp_text_input_v3_disable(Resource *resource) override;
    void zwp_text_input_v3_set_content_type(Resource *resource,
                                            uint32_t hint,
                                            uint32_t purpose) override;
    void zwp_text_input_v3_commit(Resource *resource) override;
};

class TextInputManagerV3 : public Global, public QtWaylandServer::zwp_text_input_manager_v3
{
    Q_OBJECT
public:
    explicit TextInputManagerV3(CoreCompositor *compositor, int version = 2);

    // One text input for all seats and clients; enough for a single-seat mock.
    TextInputV3 *m_textInput = nullptr;

protected:
    void zwp_text_input_manager_v3_destroy(Resource *resource) override;
    void zwp_text_input_manager_v3_get_text_input(Resource *resource,
                                                  uint32_t id,
                                                  struct ::wl_resource *seat) override;
};

} // namespace MockCompositor

#endif // MOCKCOMPOSITOR_TEXTINPUTV3_H
