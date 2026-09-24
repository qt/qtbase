// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "textinputv3.h"

namespace MockCompositor {

void TextInputV3::sendEnter(Surface *surface)
{
    wl_client *client = surface->resource()->client();
    const auto resources = resourceMap().values(client);
    for (auto *r : resources)
        send_enter(r->handle, surface->resource()->handle);
}

void TextInputV3::sendLeave(Surface *surface)
{
    wl_client *client = surface->resource()->client();
    const auto resources = resourceMap().values(client);
    for (auto *r : resources)
        send_leave(r->handle, surface->resource()->handle);
}

void TextInputV3::zwp_text_input_v3_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void TextInputV3::zwp_text_input_v3_enable(Resource *resource)
{
    Q_UNUSED(resource);
    m_pending = State();
    m_pending.enabled = true;
}

void TextInputV3::zwp_text_input_v3_disable(Resource *resource)
{
    Q_UNUSED(resource);
    m_pending = State();
}

void TextInputV3::zwp_text_input_v3_set_content_type(Resource *resource,
                                                     uint32_t hint,
                                                     uint32_t purpose)
{
    Q_UNUSED(resource);
    m_pending.contentHint = hint;
    m_pending.contentPurpose = purpose;
}

void TextInputV3::zwp_text_input_v3_commit(Resource *resource)
{
    Q_UNUSED(resource);
    m_committed = m_pending;
}

TextInputManagerV3::TextInputManagerV3(CoreCompositor *compositor, int version)
    : QtWaylandServer::zwp_text_input_manager_v3(compositor->m_display, version)
    , m_textInput(new TextInputV3)
{
    m_textInput->setParent(this);
}

void TextInputManagerV3::zwp_text_input_manager_v3_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void TextInputManagerV3::zwp_text_input_manager_v3_get_text_input(Resource *resource,
                                                                  uint32_t id,
                                                                  struct ::wl_resource *seat)
{
    Q_UNUSED(seat);
    m_textInput->add(resource->client(), id, resource->version());
}

} // namespace MockCompositor
