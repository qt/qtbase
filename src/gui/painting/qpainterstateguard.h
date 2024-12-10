// Copyright (C) 2024 Christian Ehrlicher <ch.ehrlicher@gmx.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPAINTERSTATEGUARD_H
#define QPAINTERSTATEGUARD_H

#include <QtCore/qtclasshelpermacros.h>
#include <QtGui/qpainter.h>

QT_BEGIN_NAMESPACE

class QPainterStateGuard
{
    Q_DISABLE_COPY_MOVE(QPainterStateGuard)
public:
    enum InitialState
    {
        Save,
        NoSave,
    };

    Q_NODISCARD_CTOR
    explicit QPainterStateGuard(QPainter *painter, InitialState state = Save)
        : m_painter(painter)
    {
        Q_ASSERT(painter);
        if (state == InitialState::Save)
            save();
    }

    ~QPainterStateGuard()
    {
        while (m_level > 0)
            restore();
    }

    void save()
    {
        m_painter->save();
        ++m_level;
    }

    void restore()
    {
        Q_ASSERT(m_level > 0);
        --m_level;
        m_painter->restore();
    }

private:
    QPainter *m_painter;
    int m_level = 0;
};

QT_END_NAMESPACE

#endif  // QPAINTERSTATEGUARD_H
