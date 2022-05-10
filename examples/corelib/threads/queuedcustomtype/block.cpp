// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "block.h"

Block::Block()
{
}

Block::Block(const Block &other)
    : m_rect(other.m_rect), m_color(other.m_color)
{
}

Block::~Block()
{
}

Block::Block(const QRect &rect, const QColor &color)
    : m_rect(rect), m_color(color)
{
}

QColor Block::color() const
{
    return m_color;
}

QRect Block::rect() const
{
    return m_rect;
}
