/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QCOLOR_H
#define QCOLOR_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qrgb.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qstringlist.h>
#include <QtGui/qrgba64.h>

QT_BEGIN_NAMESPACE


class QColor;
class QColormap;
class QVariant;

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QColor &);
#endif
#ifndef QT_NO_DATASTREAM
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QColor &);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QColor &);
#endif

class Q_GUI_EXPORT QColor
{
public:
    enum Spec { Invalid, Rgb, Hsv, Cmyk, Hsl, ExtendedRgb };
    enum NameFormat { HexRgb, HexArgb };

    Q_DECL_CONSTEXPR QColor() noexcept
        : cspec(Invalid), ct(USHRT_MAX, 0, 0, 0, 0) {}
    QColor(Qt::GlobalColor color) noexcept;
    Q_DECL_CONSTEXPR QColor(int r, int g, int b, int a = 255) noexcept
        : cspec(isRgbaValid(r, g, b, a) ? Rgb : Invalid),
          ct(cspec == Rgb ? a * 0x0101 : 0,
             cspec == Rgb ? r * 0x0101 : 0,
             cspec == Rgb ? g * 0x0101 : 0,
             cspec == Rgb ? b * 0x0101 : 0,
             0) {}
    QColor(QRgb rgb) noexcept;
    QColor(QRgba64 rgba64) noexcept;
#if QT_STRINGVIEW_LEVEL < 2
    inline QColor(const QString& name);
#endif
    explicit inline QColor(QStringView name);
    inline QColor(const char *aname) : QColor(QLatin1String(aname)) {}
    inline QColor(QLatin1String name);
    QColor(Spec spec) noexcept;

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    // ### Qt 6: remove all of these, the trivial ones are fine.
    Q_DECL_CONSTEXPR QColor(const QColor &color) noexcept
        : cspec(color.cspec), ct(color.ct)
    {}
    Q_DECL_CONSTEXPR QColor(QColor &&other) noexcept : cspec(other.cspec), ct(other.ct) {}
    QColor &operator=(QColor &&other) noexcept
    { cspec = other.cspec; ct = other.ct; return *this; }
    QColor &operator=(const QColor &) noexcept;
#endif // Qt < 6

    QColor &operator=(Qt::GlobalColor color) noexcept;

    bool isValid() const noexcept;

    // ### Qt 6: merge overloads
    QString name() const;
    QString name(NameFormat format) const;

#if QT_STRINGVIEW_LEVEL < 2
    void setNamedColor(const QString& name);
#endif
    void setNamedColor(QStringView name);
    void setNamedColor(QLatin1String name);

    static QStringList colorNames();

    inline Spec spec() const noexcept
    { return cspec; }

    int alpha() const noexcept;
    void setAlpha(int alpha);

    qreal alphaF() const noexcept;
    void setAlphaF(qreal alpha);

    int red() const noexcept;
    int green() const noexcept;
    int blue() const noexcept;
    void setRed(int red);
    void setGreen(int green);
    void setBlue(int blue);

    qreal redF() const noexcept;
    qreal greenF() const noexcept;
    qreal blueF() const noexcept;
    void setRedF(qreal red);
    void setGreenF(qreal green);
    void setBlueF(qreal blue);

    void getRgb(int *r, int *g, int *b, int *a = nullptr) const;
    void setRgb(int r, int g, int b, int a = 255);

    void getRgbF(qreal *r, qreal *g, qreal *b, qreal *a = nullptr) const;
    void setRgbF(qreal r, qreal g, qreal b, qreal a = 1.0);

    QRgba64 rgba64() const noexcept;
    void setRgba64(QRgba64 rgba) noexcept;

    QRgb rgba() const noexcept;
    void setRgba(QRgb rgba) noexcept;

    QRgb rgb() const noexcept;
    void setRgb(QRgb rgb) noexcept;

    int hue() const noexcept; // 0 <= hue < 360
    int saturation() const noexcept;
    int hsvHue() const noexcept; // 0 <= hue < 360
    int hsvSaturation() const noexcept;
    int value() const noexcept;

    qreal hueF() const noexcept; // 0.0 <= hueF < 360.0
    qreal saturationF() const noexcept;
    qreal hsvHueF() const noexcept; // 0.0 <= hueF < 360.0
    qreal hsvSaturationF() const noexcept;
    qreal valueF() const noexcept;

    void getHsv(int *h, int *s, int *v, int *a = nullptr) const;
    void setHsv(int h, int s, int v, int a = 255);

    void getHsvF(qreal *h, qreal *s, qreal *v, qreal *a = nullptr) const;
    void setHsvF(qreal h, qreal s, qreal v, qreal a = 1.0);

    int cyan() const noexcept;
    int magenta() const noexcept;
    int yellow() const noexcept;
    int black() const noexcept;

    qreal cyanF() const noexcept;
    qreal magentaF() const noexcept;
    qreal yellowF() const noexcept;
    qreal blackF() const noexcept;

    void getCmyk(int *c, int *m, int *y, int *k, int *a = nullptr); // ### Qt 6: remove
    void getCmyk(int *c, int *m, int *y, int *k, int *a = nullptr) const;
    void setCmyk(int c, int m, int y, int k, int a = 255);

    void getCmykF(qreal *c, qreal *m, qreal *y, qreal *k, qreal *a = nullptr); // ### Qt 6: remove
    void getCmykF(qreal *c, qreal *m, qreal *y, qreal *k, qreal *a = nullptr) const;
    void setCmykF(qreal c, qreal m, qreal y, qreal k, qreal a = 1.0);

    int hslHue() const noexcept; // 0 <= hue < 360
    int hslSaturation() const noexcept;
    int lightness() const noexcept;

    qreal hslHueF() const noexcept; // 0.0 <= hueF < 360.0
    qreal hslSaturationF() const noexcept;
    qreal lightnessF() const noexcept;

    void getHsl(int *h, int *s, int *l, int *a = nullptr) const;
    void setHsl(int h, int s, int l, int a = 255);

    void getHslF(qreal *h, qreal *s, qreal *l, qreal *a = nullptr) const;
    void setHslF(qreal h, qreal s, qreal l, qreal a = 1.0);

    QColor toRgb() const noexcept;
    QColor toHsv() const noexcept;
    QColor toCmyk() const noexcept;
    QColor toHsl() const noexcept;
    QColor toExtendedRgb() const noexcept;

    Q_REQUIRED_RESULT QColor convertTo(Spec colorSpec) const noexcept;

    static QColor fromRgb(QRgb rgb) noexcept;
    static QColor fromRgba(QRgb rgba) noexcept;

    static QColor fromRgb(int r, int g, int b, int a = 255);
    static QColor fromRgbF(qreal r, qreal g, qreal b, qreal a = 1.0);

    static QColor fromRgba64(ushort r, ushort g, ushort b, ushort a = USHRT_MAX) noexcept;
    static QColor fromRgba64(QRgba64 rgba) noexcept;

    static QColor fromHsv(int h, int s, int v, int a = 255);
    static QColor fromHsvF(qreal h, qreal s, qreal v, qreal a = 1.0);

    static QColor fromCmyk(int c, int m, int y, int k, int a = 255);
    static QColor fromCmykF(qreal c, qreal m, qreal y, qreal k, qreal a = 1.0);

    static QColor fromHsl(int h, int s, int l, int a = 255);
    static QColor fromHslF(qreal h, qreal s, qreal l, qreal a = 1.0);

#if QT_DEPRECATED_SINCE(5, 13)
    QT_DEPRECATED_X("Use QColor::lighter() instead")
    Q_REQUIRED_RESULT QColor light(int f = 150) const noexcept;
    QT_DEPRECATED_X("Use QColor::darker() instead")
    Q_REQUIRED_RESULT QColor dark(int f = 200) const noexcept;
#endif
    Q_REQUIRED_RESULT QColor lighter(int f = 150) const noexcept;
    Q_REQUIRED_RESULT QColor darker(int f = 200) const noexcept;

    bool operator==(const QColor &c) const noexcept;
    bool operator!=(const QColor &c) const noexcept;

    operator QVariant() const;

#if QT_STRINGVIEW_LEVEL < 2
    static bool isValidColor(const QString &name);
#endif
    static bool isValidColor(QStringView) noexcept;
    static bool isValidColor(QLatin1String) noexcept;

private:

    void invalidate() noexcept;
    template <typename String>
    bool setColorFromString(String name);

    static Q_DECL_CONSTEXPR bool isRgbaValid(int r, int g, int b, int a = 255) noexcept Q_DECL_CONST_FUNCTION
    {
        return uint(r) <= 255 && uint(g) <= 255 && uint(b) <= 255 && uint(a) <= 255;
    }

    Spec cspec;
    union CT {
#ifdef Q_COMPILER_UNIFORM_INIT
        CT() {} // doesn't init anything, thus can't be constexpr
        Q_DECL_CONSTEXPR explicit CT(ushort a1, ushort a2, ushort a3, ushort a4, ushort a5) noexcept
            : array{a1, a2, a3, a4, a5} {}
#endif
        struct {
            ushort alpha;
            ushort red;
            ushort green;
            ushort blue;
            ushort pad;
        } argb;
        struct {
            ushort alpha;
            ushort hue;
            ushort saturation;
            ushort value;
            ushort pad;
        } ahsv;
        struct {
            ushort alpha;
            ushort cyan;
            ushort magenta;
            ushort yellow;
            ushort black;
        } acmyk;
        struct {
            ushort alpha;
            ushort hue;
            ushort saturation;
            ushort lightness;
            ushort pad;
        } ahsl;
        struct {
            ushort alphaF16;
            ushort redF16;
            ushort greenF16;
            ushort blueF16;
            ushort pad;
        } argbExtended;
        ushort array[5];
    } ct;

    friend class QColormap;
#ifndef QT_NO_DATASTREAM
    friend Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QColor &);
    friend Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QColor &);
#endif
};
Q_DECLARE_TYPEINFO(QColor, QT_VERSION >= QT_VERSION_CHECK(6,0,0) ? Q_MOVABLE_TYPE : Q_RELOCATABLE_TYPE);

inline QColor::QColor(QLatin1String aname)
{ setNamedColor(aname); }

inline QColor::QColor(QStringView aname)
{ setNamedColor(aname); }

#if QT_STRINGVIEW_LEVEL < 2
inline QColor::QColor(const QString& aname)
{ setNamedColor(aname); }
#endif

inline bool QColor::isValid() const noexcept
{ return cspec != Invalid; }

QT_END_NAMESPACE

#endif // QCOLOR_H
