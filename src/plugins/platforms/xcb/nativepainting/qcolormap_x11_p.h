// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <QColor>
#include <QList>

QT_BEGIN_NAMESPACE

class QXcbColormapPrivate;
class QXcbColormap
{
public:
    enum Mode { Direct, Indexed, Gray };

    static void initialize();
    static void cleanup();

    static QXcbColormap instance(int screen = -1);

    QXcbColormap(const QXcbColormap &colormap);
    ~QXcbColormap();

    QXcbColormap &operator=(const QXcbColormap &colormap);

    Mode mode() const;

    int depth() const;
    int size() const;

    uint pixel(const QColor &color) const;
    const QColor colorAt(uint pixel) const;

    const QList<QColor> colormap() const;

private:
    QXcbColormap();
    QXcbColormapPrivate *d;
};

QT_END_NAMESPACE
