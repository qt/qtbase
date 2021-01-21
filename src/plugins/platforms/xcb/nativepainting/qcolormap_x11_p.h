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

#ifndef QCOLORMAP_X11_H
#define QCOLORMAP_X11_H

#include <QColor>
#include <QVector>

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

    const QVector<QColor> colormap() const;

private:
    QXcbColormap();
    QXcbColormapPrivate *d;
};

QT_END_NAMESPACE

#endif // QCOLORMAP_X11_H
