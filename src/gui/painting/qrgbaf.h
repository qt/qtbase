/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QRGBAF_H
#define QRGBAF_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qfloat16.h>

#include <algorithm>
#include <cmath>

QT_BEGIN_NAMESPACE

template<typename F>
class alignas(sizeof(F) * 4) QRgbaF {
public:
    F r;
    F g;
    F b;
    F a;

    static constexpr
    QRgbaF fromRgba64(quint16 red, quint16 green, quint16 blue, quint16 alpha)
    {
        return QRgbaF{
            red   * (1.0f / 65535.0f),
            green * (1.0f / 65535.0f),
            blue  * (1.0f / 65535.0f),
            alpha * (1.0f / 65535.0f) };
    }

    static constexpr
    QRgbaF fromRgba(quint8 red, quint8 green, quint8 blue, quint8 alpha)
    {
        return QRgbaF{
            red   * (1.0f / 255.0f),
            green * (1.0f / 255.0f),
            blue  * (1.0f / 255.0f),
            alpha * (1.0f / 255.0f) };
    }
    static constexpr
    QRgbaF fromArgb32(uint rgb)
    {
        return fromRgba(quint8(rgb >> 16), quint8(rgb >> 8), quint8(rgb), quint8(rgb >> 24));
    }

    constexpr bool isOpaque() const { return a >= 1.0f; }
    constexpr bool isTransparent() const { return a <= 0.0f; }

    constexpr float red()   const { return r; }
    constexpr float green() const { return g; }
    constexpr float blue()  const { return b; }
    constexpr float alpha() const { return a; }
    void setRed(float _red)     { r = _red; }
    void setGreen(float _green) { g = _green; }
    void setBlue(float _blue)   { b = _blue; }
    void setAlpha(float _alpha) { a = _alpha; }

    constexpr float redNormalized()   const { return std::clamp(static_cast<float>(r), 0.0f, 1.0f); }
    constexpr float greenNormalized() const { return std::clamp(static_cast<float>(g), 0.0f, 1.0f); }
    constexpr float blueNormalized()  const { return std::clamp(static_cast<float>(b), 0.0f, 1.0f); }
    constexpr float alphaNormalized() const { return std::clamp(static_cast<float>(a), 0.0f, 1.0f); }

    constexpr quint8 red8()   const { return std::lround(redNormalized()   * 255.0f); }
    constexpr quint8 green8() const { return std::lround(greenNormalized() * 255.0f); }
    constexpr quint8 blue8()  const { return std::lround(blueNormalized()  * 255.0f); }
    constexpr quint8 alpha8() const { return std::lround(alphaNormalized() * 255.0f); }
    constexpr uint toArgb32() const
    {
       return uint((alpha8() << 24) | (red8() << 16) | (green8() << 8) | blue8());
    }

    constexpr quint16 red16()   const { return std::lround(redNormalized()   * 65535.0f); }
    constexpr quint16 green16() const { return std::lround(greenNormalized() * 65535.0f); }
    constexpr quint16 blue16()  const { return std::lround(blueNormalized()  * 65535.0f); }
    constexpr quint16 alpha16() const { return std::lround(alphaNormalized() * 65535.0f); }

    constexpr Q_ALWAYS_INLINE QRgbaF premultiplied() const
    {
        return QRgbaF{r * a, g * a, b * a, a};
    }
    constexpr Q_ALWAYS_INLINE QRgbaF unpremultiplied() const
    {
        if (a <= 0.0f)
            return QRgbaF{0.0f, 0.0f, 0.0f, 0.0f};
        if (a >= 1.0f)
            return *this;
        const float ia = 1.0f / a;
        return QRgbaF{r * ia, g * ia, b * ia, a};
    }
};

typedef QRgbaF<float> QRgba32F;
typedef QRgbaF<qfloat16> QRgba16F;

QT_END_NAMESPACE

#endif // QRGBAF_H
