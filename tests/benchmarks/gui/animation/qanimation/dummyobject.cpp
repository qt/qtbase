// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "dummyobject.h"

DummyObject::DummyObject()
{
}

QRect DummyObject::rect() const
{
    return m_rect;
}

void DummyObject::setRect(const QRect &r)
{
    m_rect = r;
}

float DummyObject::opacity() const
{
    return m_opacity;
}

void DummyObject::setOpacity(float o)
{
    m_opacity = o;
}
