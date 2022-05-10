// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef BLOCK_H
#define BLOCK_H

#include <QColor>
#include <QMetaType>
#include <QRect>

//! [custom type definition and meta-type declaration]
class Block
{
public:
    Block();
    Block(const Block &other);
    ~Block();

    Block(const QRect &rect, const QColor &color);

    QColor color() const;
    QRect rect() const;

private:
    QRect m_rect;
    QColor m_color;
};

Q_DECLARE_METATYPE(Block);
//! [custom type definition and meta-type declaration]

#endif
