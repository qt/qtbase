/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QRGBA64_H
#define QRGBA64_H

#include <QtCore/qglobal.h>
#include <QtCore/qprocessordetection.h>

QT_BEGIN_NAMESPACE

class QRgba64 {
    union {
        struct {
            quint16 red;
            quint16 green;
            quint16 blue;
            quint16 alpha;
        } c;
        quint64 rgba;
    };
public:
    // No constructors are allowed, since this needs to be usable in a union in no-c++11 mode.
    // When c++11 is mandatory, we can add all but a copy constructor.
    Q_DECL_RELAXED_CONSTEXPR static QRgba64 fromRgba64(quint16 red, quint16 green, quint16 blue, quint16 alpha)
    {
        QRgba64 rgba64
#ifdef Q_COMPILER_UNIFORM_INIT
            = {}
#endif
        ;

        rgba64.c.red = red;
        rgba64.c.green = green;
        rgba64.c.blue = blue;
        rgba64.c.alpha = alpha;
        return rgba64;
    }
    Q_DECL_RELAXED_CONSTEXPR static QRgba64 fromRgba64(quint64 c)
    {
        QRgba64 rgba64
#ifdef Q_COMPILER_UNIFORM_INIT
            = {}
#endif
        ;
        rgba64.rgba = c;
        return rgba64;
    }
    Q_DECL_RELAXED_CONSTEXPR static QRgba64 fromRgba(quint8 red, quint8 green, quint8 blue, quint8 alpha)
    {
        QRgba64 rgb64 = fromRgba64(red, green, blue, alpha);
        // Expand the range so that 0x00 maps to 0x0000 and 0xff maps to 0xffff.
        rgb64.rgba |= rgb64.rgba << 8;
        return rgb64;
    }
    Q_DECL_RELAXED_CONSTEXPR static QRgba64 fromArgb32(uint rgb)
    {
        return fromRgba(rgb >> 16, rgb >> 8, rgb, rgb >> 24);
    }

    Q_DECL_CONSTEXPR quint16 red()   const { return c.red; }
    Q_DECL_CONSTEXPR quint16 green() const { return c.green; }
    Q_DECL_CONSTEXPR quint16 blue()  const { return c.blue; }
    Q_DECL_CONSTEXPR quint16 alpha() const { return c.alpha; }
    void setRed(quint16 _red) { c.red = _red; }
    void setGreen(quint16 _green) { c.green = _green; }
    void setBlue(quint16 _blue) { c.blue = _blue; }
    void setAlpha(quint16 _alpha) { c.alpha = _alpha; }

    Q_DECL_CONSTEXPR quint8 red8()   const { return div_257(c.red); }
    Q_DECL_CONSTEXPR quint8 green8() const { return div_257(c.green); }
    Q_DECL_CONSTEXPR quint8 blue8()  const { return div_257(c.blue); }
    Q_DECL_CONSTEXPR quint8 alpha8() const { return div_257(c.alpha); }
    Q_DECL_CONSTEXPR uint toArgb32() const
    {
        return (alpha8() << 24) | (red8() << 16) | (green8() << 8) | blue8();
    }
    Q_DECL_CONSTEXPR ushort toRgb16() const
    {
        return (c.red & 0xf800) | ((c.green >> 10) << 5) | (c.blue >> 11);
    }

    Q_DECL_RELAXED_CONSTEXPR QRgba64 premultiplied() const
    {
        const quint32 a = c.alpha;
        const quint16 r = div_65535(c.red   * a);
        const quint16 g = div_65535(c.green * a);
        const quint16 b = div_65535(c.blue  * a);
        return fromRgba64(r, g, b, a);
    }

    Q_DECL_RELAXED_CONSTEXPR QRgba64 unpremultiplied() const
    {
#if Q_PROCESSOR_WORDSIZE < 8
        return unpremultiplied_32bit();
#else
        return unpremultiplied_64bit();
#endif
    }

    Q_DECL_CONSTEXPR operator quint64() const
    {
        return rgba;
    }

private:
    static Q_DECL_CONSTEXPR uint div_257_floor(uint x) { return  (x - (x >> 8)) >> 8; }
    static Q_DECL_CONSTEXPR uint div_257(uint x) { return div_257_floor(x + 128); }
    static Q_DECL_CONSTEXPR uint div_65535(uint x) { return (x + (x>>16) + 0x8000U) >> 16; }
    Q_DECL_RELAXED_CONSTEXPR QRgba64 unpremultiplied_32bit() const
    {
        if (c.alpha == 0xffff || c.alpha == 0)
            return *this;
        const quint16 r = (quint32(c.red) * 0xffff + c.alpha/2) / c.alpha;
        const quint16 g = (quint32(c.green) * 0xffff + c.alpha/2) / c.alpha;
        const quint16 b = (quint32(c.blue) * 0xffff + c.alpha/2) / c.alpha;
        return fromRgba64(r, g, b, c.alpha);
    }
    Q_DECL_RELAXED_CONSTEXPR QRgba64 unpremultiplied_64bit() const
    {
        if (c.alpha == 0xffff || c.alpha == 0)
            return *this;
        const quint64 fa = (Q_UINT64_C(0xffff00008000) + c.alpha/2) / c.alpha;
        const quint16 r = (c.red   * fa + 0x80000000) >> 32;
        const quint16 g = (c.green * fa + 0x80000000) >> 32;
        const quint16 b = (c.blue  * fa + 0x80000000) >> 32;
        return fromRgba64(r, g, b, c.alpha);
    }
};

Q_DECL_RELAXED_CONSTEXPR inline QRgba64 qRgba64(quint16 r, quint16 g, quint16 b, quint16 a)
{
    return QRgba64::fromRgba64(r, g, b, a);
}

Q_DECL_RELAXED_CONSTEXPR inline QRgba64 qRgba64(quint64 c)
{
    return QRgba64::fromRgba64(c);
}

Q_DECL_RELAXED_CONSTEXPR inline QRgba64 qPremultiply(QRgba64 c)
{
    return c.premultiplied();
}

Q_DECL_RELAXED_CONSTEXPR inline QRgba64 qUnpremultiply(QRgba64 c)
{
    return c.unpremultiplied();
}

inline Q_DECL_CONSTEXPR uint qRed(QRgba64 rgb)
{ return rgb.red8(); }

inline Q_DECL_CONSTEXPR uint qGreen(QRgba64 rgb)
{ return rgb.green8(); }

inline Q_DECL_CONSTEXPR uint qBlue(QRgba64 rgb)
{ return rgb.blue8(); }

inline Q_DECL_CONSTEXPR uint qAlpha(QRgba64 rgb)
{ return rgb.alpha8(); }

QT_END_NAMESPACE

#endif // QRGBA64_H
