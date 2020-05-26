/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <limits.h>

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

    constexpr QColor() noexcept
        : cspec(Invalid), ct(USHRT_MAX, 0, 0, 0, 0) {}
    QColor(Qt::GlobalColor color) noexcept;
    constexpr QColor(int r, int g, int b, int a = 255) noexcept
        : cspec(isRgbaValid(r, g, b, a) ? Rgb : Invalid),
          ct(ushort(cspec == Rgb ? a * 0x0101 : 0),
             ushort(cspec == Rgb ? r * 0x0101 : 0),
             ushort(cspec == Rgb ? g * 0x0101 : 0),
             ushort(cspec == Rgb ? b * 0x0101 : 0),
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

    QColor &operator=(Qt::GlobalColor color) noexcept;

    bool isValid() const noexcept;

    QString name(NameFormat format = HexRgb) const;

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

    float alphaF() const noexcept;
    void setAlphaF(float alpha);

    int red() const noexcept;
    int green() const noexcept;
    int blue() const noexcept;
    void setRed(int red);
    void setGreen(int green);
    void setBlue(int blue);

    float redF() const noexcept;
    float greenF() const noexcept;
    float blueF() const noexcept;
    void setRedF(float red);
    void setGreenF(float green);
    void setBlueF(float blue);

    void getRgb(int *r, int *g, int *b, int *a = nullptr) const;
    void setRgb(int r, int g, int b, int a = 255);

    void getRgbF(float *r, float *g, float *b, float *a = nullptr) const;
    void setRgbF(float r, float g, float b, float a = 1.0);

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

    float hueF() const noexcept; // 0.0 <= hueF < 360.0
    float saturationF() const noexcept;
    float hsvHueF() const noexcept; // 0.0 <= hueF < 360.0
    float hsvSaturationF() const noexcept;
    float valueF() const noexcept;

    void getHsv(int *h, int *s, int *v, int *a = nullptr) const;
    void setHsv(int h, int s, int v, int a = 255);

    void getHsvF(float *h, float *s, float *v, float *a = nullptr) const;
    void setHsvF(float h, float s, float v, float a = 1.0);

    int cyan() const noexcept;
    int magenta() const noexcept;
    int yellow() const noexcept;
    int black() const noexcept;

    float cyanF() const noexcept;
    float magentaF() const noexcept;
    float yellowF() const noexcept;
    float blackF() const noexcept;

    void getCmyk(int *c, int *m, int *y, int *k, int *a = nullptr) const;
    void setCmyk(int c, int m, int y, int k, int a = 255);

    void getCmykF(float *c, float *m, float *y, float *k, float *a = nullptr) const;
    void setCmykF(float c, float m, float y, float k, float a = 1.0);

    int hslHue() const noexcept; // 0 <= hue < 360
    int hslSaturation() const noexcept;
    int lightness() const noexcept;

    float hslHueF() const noexcept; // 0.0 <= hueF < 360.0
    float hslSaturationF() const noexcept;
    float lightnessF() const noexcept;

    void getHsl(int *h, int *s, int *l, int *a = nullptr) const;
    void setHsl(int h, int s, int l, int a = 255);

    void getHslF(float *h, float *s, float *l, float *a = nullptr) const;
    void setHslF(float h, float s, float l, float a = 1.0);

    QColor toRgb() const noexcept;
    QColor toHsv() const noexcept;
    QColor toCmyk() const noexcept;
    QColor toHsl() const noexcept;
    QColor toExtendedRgb() const noexcept;

    Q_REQUIRED_RESULT QColor convertTo(Spec colorSpec) const noexcept;

    static QColor fromRgb(QRgb rgb) noexcept;
    static QColor fromRgba(QRgb rgba) noexcept;

    static QColor fromRgb(int r, int g, int b, int a = 255);
    static QColor fromRgbF(float r, float g, float b, float a = 1.0);

    static QColor fromRgba64(ushort r, ushort g, ushort b, ushort a = USHRT_MAX) noexcept;
    static QColor fromRgba64(QRgba64 rgba) noexcept;

    static QColor fromHsv(int h, int s, int v, int a = 255);
    static QColor fromHsvF(float h, float s, float v, float a = 1.0);

    static QColor fromCmyk(int c, int m, int y, int k, int a = 255);
    static QColor fromCmykF(float c, float m, float y, float k, float a = 1.0);

    static QColor fromHsl(int h, int s, int l, int a = 255);
    static QColor fromHslF(float h, float s, float l, float a = 1.0);

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

    static constexpr bool isRgbaValid(int r, int g, int b, int a = 255) noexcept Q_DECL_CONST_FUNCTION
    {
        return uint(r) <= 255 && uint(g) <= 255 && uint(b) <= 255 && uint(a) <= 255;
    }

    Spec cspec;
    union CT {
#ifdef Q_COMPILER_UNIFORM_INIT
        CT() {} // doesn't init anything, thus can't be constexpr
        constexpr explicit CT(ushort a1, ushort a2, ushort a3, ushort a4, ushort a5) noexcept
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

#ifdef Q_COMPILER_UNIFORM_INIT
public: // can't give friendship to a namespace, so it needs to be public
    constexpr explicit QColor(Spec spec, ushort a1, ushort a2, ushort a3, ushort a4, ushort a5=0) noexcept
        : cspec(spec), ct(a1, a2, a3, a4, a5) {}
#endif // Q_COMPILER_UNIFORM_INIT
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

// define these namespaces even if the contents are ifdef'd out
namespace QColorConstants
{
namespace Svg {}

#if defined(Q_COMPILER_CONSTEXPR) & defined(Q_COMPILER_UNIFORM_INIT)
    // Qt::GlobalColor names
    Q_DECL_UNUSED constexpr QColor Color0      {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0x00 * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor Color1      {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xff * 0x101, 0xff * 0x101};
    Q_DECL_UNUSED constexpr QColor Black       {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0x00 * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor White       {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xff * 0x101, 0xff * 0x101};
    Q_DECL_UNUSED constexpr QColor DarkGray    {QColor::Rgb, 0xff * 0x101, 0x80 * 0x101, 0x80 * 0x101, 0x80 * 0x101};
    Q_DECL_UNUSED constexpr QColor Gray        {QColor::Rgb, 0xff * 0x101, 0xa0 * 0x101, 0xa0 * 0x101, 0xa4 * 0x101};
    Q_DECL_UNUSED constexpr QColor LightGray   {QColor::Rgb, 0xff * 0x101, 0xc0 * 0x101, 0xc0 * 0x101, 0xc0 * 0x101};
    Q_DECL_UNUSED constexpr QColor Red         {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0x00 * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor Green       {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0xff * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor Blue        {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0x00 * 0x101, 0xff * 0x101};
    Q_DECL_UNUSED constexpr QColor Cyan        {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0xff * 0x101, 0xff * 0x101};
    Q_DECL_UNUSED constexpr QColor Magenta     {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0x00 * 0x101, 0xff * 0x101};
    Q_DECL_UNUSED constexpr QColor Yellow      {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xff * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor DarkRed     {QColor::Rgb, 0xff * 0x101, 0x80 * 0x101, 0x00 * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor DarkGreen   {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0x80 * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor DarkBlue    {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0x00 * 0x101, 0x80 * 0x101};
    Q_DECL_UNUSED constexpr QColor DarkCyan    {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0x80 * 0x101, 0x80 * 0x101};
    Q_DECL_UNUSED constexpr QColor DarkMagenta {QColor::Rgb, 0xff * 0x101, 0x80 * 0x101, 0x00 * 0x101, 0x80 * 0x101};
    Q_DECL_UNUSED constexpr QColor DarkYellow  {QColor::Rgb, 0xff * 0x101, 0x80 * 0x101, 0x80 * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor Transparent {QColor::Rgb, 0x00 * 0x101, 0x00 * 0x101, 0x00 * 0x101, 0x00 * 0x101};

    // SVG names supported by QColor (see qcolor.cpp).
namespace Svg {
    Q_DECL_UNUSED constexpr QColor aliceblue                {QColor::Rgb, 0xff * 0x101, 0xf0 * 0x101, 0xf8 * 0x101, 0xff * 0x101};
    Q_DECL_UNUSED constexpr QColor antiquewhite             {QColor::Rgb, 0xff * 0x101, 0xfa * 0x101, 0xeb * 0x101, 0xd7 * 0x101};
    Q_DECL_UNUSED constexpr QColor aqua                     {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0xff * 0x101, 0xff * 0x101};
    Q_DECL_UNUSED constexpr QColor aquamarine               {QColor::Rgb, 0xff * 0x101, 0x7f * 0x101, 0xff * 0x101, 0xd4 * 0x101};
    Q_DECL_UNUSED constexpr QColor azure                    {QColor::Rgb, 0xff * 0x101, 0xf0 * 0x101, 0xff * 0x101, 0xff * 0x101};
    Q_DECL_UNUSED constexpr QColor beige                    {QColor::Rgb, 0xff * 0x101, 0xf5 * 0x101, 0xf5 * 0x101, 0xdc * 0x101};
    Q_DECL_UNUSED constexpr QColor bisque                   {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xe4 * 0x101, 0xc4 * 0x101};
    Q_DECL_UNUSED constexpr QColor black                    {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0x00 * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor blanchedalmond           {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xeb * 0x101, 0xcd * 0x101};
    Q_DECL_UNUSED constexpr QColor blue                     {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0x00 * 0x101, 0xff * 0x101};
    Q_DECL_UNUSED constexpr QColor blueviolet               {QColor::Rgb, 0xff * 0x101, 0x8a * 0x101, 0x2b * 0x101, 0xe2 * 0x101};
    Q_DECL_UNUSED constexpr QColor brown                    {QColor::Rgb, 0xff * 0x101, 0xa5 * 0x101, 0x2a * 0x101, 0x2a * 0x101};
    Q_DECL_UNUSED constexpr QColor burlywood                {QColor::Rgb, 0xff * 0x101, 0xde * 0x101, 0xb8 * 0x101, 0x87 * 0x101};
    Q_DECL_UNUSED constexpr QColor cadetblue                {QColor::Rgb, 0xff * 0x101, 0x5f * 0x101, 0x9e * 0x101, 0xa0 * 0x101};
    Q_DECL_UNUSED constexpr QColor chartreuse               {QColor::Rgb, 0xff * 0x101, 0x7f * 0x101, 0xff * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor chocolate                {QColor::Rgb, 0xff * 0x101, 0xd2 * 0x101, 0x69 * 0x101, 0x1e * 0x101};
    Q_DECL_UNUSED constexpr QColor coral                    {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0x7f * 0x101, 0x50 * 0x101};
    Q_DECL_UNUSED constexpr QColor cornflowerblue           {QColor::Rgb, 0xff * 0x101, 0x64 * 0x101, 0x95 * 0x101, 0xed * 0x101};
    Q_DECL_UNUSED constexpr QColor cornsilk                 {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xf8 * 0x101, 0xdc * 0x101};
    Q_DECL_UNUSED constexpr QColor crimson                  {QColor::Rgb, 0xff * 0x101, 0xdc * 0x101, 0x14 * 0x101, 0x3c * 0x101};
    Q_DECL_UNUSED constexpr QColor cyan                     {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0xff * 0x101, 0xff * 0x101};
    Q_DECL_UNUSED constexpr QColor darkblue                 {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0x00 * 0x101, 0x8b * 0x101};
    Q_DECL_UNUSED constexpr QColor darkcyan                 {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0x8b * 0x101, 0x8b * 0x101};
    Q_DECL_UNUSED constexpr QColor darkgoldenrod            {QColor::Rgb, 0xff * 0x101, 0xb8 * 0x101, 0x86 * 0x101, 0x0b * 0x101};
    Q_DECL_UNUSED constexpr QColor darkgray                 {QColor::Rgb, 0xff * 0x101, 0xa9 * 0x101, 0xa9 * 0x101, 0xa9 * 0x101};
    Q_DECL_UNUSED constexpr QColor darkgreen                {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0x64 * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor darkgrey                 {QColor::Rgb, 0xff * 0x101, 0xa9 * 0x101, 0xa9 * 0x101, 0xa9 * 0x101};
    Q_DECL_UNUSED constexpr QColor darkkhaki                {QColor::Rgb, 0xff * 0x101, 0xbd * 0x101, 0xb7 * 0x101, 0x6b * 0x101};
    Q_DECL_UNUSED constexpr QColor darkmagenta              {QColor::Rgb, 0xff * 0x101, 0x8b * 0x101, 0x00 * 0x101, 0x8b * 0x101};
    Q_DECL_UNUSED constexpr QColor darkolivegreen           {QColor::Rgb, 0xff * 0x101, 0x55 * 0x101, 0x6b * 0x101, 0x2f * 0x101};
    Q_DECL_UNUSED constexpr QColor darkorange               {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0x8c * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor darkorchid               {QColor::Rgb, 0xff * 0x101, 0x99 * 0x101, 0x32 * 0x101, 0xcc * 0x101};
    Q_DECL_UNUSED constexpr QColor darkred                  {QColor::Rgb, 0xff * 0x101, 0x8b * 0x101, 0x00 * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor darksalmon               {QColor::Rgb, 0xff * 0x101, 0xe9 * 0x101, 0x96 * 0x101, 0x7a * 0x101};
    Q_DECL_UNUSED constexpr QColor darkseagreen             {QColor::Rgb, 0xff * 0x101, 0x8f * 0x101, 0xbc * 0x101, 0x8f * 0x101};
    Q_DECL_UNUSED constexpr QColor darkslateblue            {QColor::Rgb, 0xff * 0x101, 0x48 * 0x101, 0x3d * 0x101, 0x8b * 0x101};
    Q_DECL_UNUSED constexpr QColor darkslategray            {QColor::Rgb, 0xff * 0x101, 0x2f * 0x101, 0x4f * 0x101, 0x4f * 0x101};
    Q_DECL_UNUSED constexpr QColor darkslategrey            {QColor::Rgb, 0xff * 0x101, 0x2f * 0x101, 0x4f * 0x101, 0x4f * 0x101};
    Q_DECL_UNUSED constexpr QColor darkturquoise            {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0xce * 0x101, 0xd1 * 0x101};
    Q_DECL_UNUSED constexpr QColor darkviolet               {QColor::Rgb, 0xff * 0x101, 0x94 * 0x101, 0x00 * 0x101, 0xd3 * 0x101};
    Q_DECL_UNUSED constexpr QColor deeppink                 {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0x14 * 0x101, 0x93 * 0x101};
    Q_DECL_UNUSED constexpr QColor deepskyblue              {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0xbf * 0x101, 0xff * 0x101};
    Q_DECL_UNUSED constexpr QColor dimgray                  {QColor::Rgb, 0xff * 0x101, 0x69 * 0x101, 0x69 * 0x101, 0x69 * 0x101};
    Q_DECL_UNUSED constexpr QColor dimgrey                  {QColor::Rgb, 0xff * 0x101, 0x69 * 0x101, 0x69 * 0x101, 0x69 * 0x101};
    Q_DECL_UNUSED constexpr QColor dodgerblue               {QColor::Rgb, 0xff * 0x101, 0x1e * 0x101, 0x90 * 0x101, 0xff * 0x101};
    Q_DECL_UNUSED constexpr QColor firebrick                {QColor::Rgb, 0xff * 0x101, 0xb2 * 0x101, 0x22 * 0x101, 0x22 * 0x101};
    Q_DECL_UNUSED constexpr QColor floralwhite              {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xfa * 0x101, 0xf0 * 0x101};
    Q_DECL_UNUSED constexpr QColor forestgreen              {QColor::Rgb, 0xff * 0x101, 0x22 * 0x101, 0x8b * 0x101, 0x22 * 0x101};
    Q_DECL_UNUSED constexpr QColor fuchsia                  {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0x00 * 0x101, 0xff * 0x101};
    Q_DECL_UNUSED constexpr QColor gainsboro                {QColor::Rgb, 0xff * 0x101, 0xdc * 0x101, 0xdc * 0x101, 0xdc * 0x101};
    Q_DECL_UNUSED constexpr QColor ghostwhite               {QColor::Rgb, 0xff * 0x101, 0xf8 * 0x101, 0xf8 * 0x101, 0xff * 0x101};
    Q_DECL_UNUSED constexpr QColor gold                     {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xd7 * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor goldenrod                {QColor::Rgb, 0xff * 0x101, 0xda * 0x101, 0xa5 * 0x101, 0x20 * 0x101};
    Q_DECL_UNUSED constexpr QColor gray                     {QColor::Rgb, 0xff * 0x101, 0x80 * 0x101, 0x80 * 0x101, 0x80 * 0x101};
    Q_DECL_UNUSED constexpr QColor green                    {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0x80 * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor greenyellow              {QColor::Rgb, 0xff * 0x101, 0xad * 0x101, 0xff * 0x101, 0x2f * 0x101};
    Q_DECL_UNUSED constexpr QColor grey                     {QColor::Rgb, 0xff * 0x101, 0x80 * 0x101, 0x80 * 0x101, 0x80 * 0x101};
    Q_DECL_UNUSED constexpr QColor honeydew                 {QColor::Rgb, 0xff * 0x101, 0xf0 * 0x101, 0xff * 0x101, 0xf0 * 0x101};
    Q_DECL_UNUSED constexpr QColor hotpink                  {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0x69 * 0x101, 0xb4 * 0x101};
    Q_DECL_UNUSED constexpr QColor indianred                {QColor::Rgb, 0xff * 0x101, 0xcd * 0x101, 0x5c * 0x101, 0x5c * 0x101};
    Q_DECL_UNUSED constexpr QColor indigo                   {QColor::Rgb, 0xff * 0x101, 0x4b * 0x101, 0x00 * 0x101, 0x82 * 0x101};
    Q_DECL_UNUSED constexpr QColor ivory                    {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xff * 0x101, 0xf0 * 0x101};
    Q_DECL_UNUSED constexpr QColor khaki                    {QColor::Rgb, 0xff * 0x101, 0xf0 * 0x101, 0xe6 * 0x101, 0x8c * 0x101};
    Q_DECL_UNUSED constexpr QColor lavender                 {QColor::Rgb, 0xff * 0x101, 0xe6 * 0x101, 0xe6 * 0x101, 0xfa * 0x101};
    Q_DECL_UNUSED constexpr QColor lavenderblush            {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xf0 * 0x101, 0xf5 * 0x101};
    Q_DECL_UNUSED constexpr QColor lawngreen                {QColor::Rgb, 0xff * 0x101, 0x7c * 0x101, 0xfc * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor lemonchiffon             {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xfa * 0x101, 0xcd * 0x101};
    Q_DECL_UNUSED constexpr QColor lightblue                {QColor::Rgb, 0xff * 0x101, 0xad * 0x101, 0xd8 * 0x101, 0xe6 * 0x101};
    Q_DECL_UNUSED constexpr QColor lightcoral               {QColor::Rgb, 0xff * 0x101, 0xf0 * 0x101, 0x80 * 0x101, 0x80 * 0x101};
    Q_DECL_UNUSED constexpr QColor lightcyan                {QColor::Rgb, 0xff * 0x101, 0xe0 * 0x101, 0xff * 0x101, 0xff * 0x101};
    Q_DECL_UNUSED constexpr QColor lightgoldenrodyellow     {QColor::Rgb, 0xff * 0x101, 0xfa * 0x101, 0xfa * 0x101, 0xd2 * 0x101};
    Q_DECL_UNUSED constexpr QColor lightgray                {QColor::Rgb, 0xff * 0x101, 0xd3 * 0x101, 0xd3 * 0x101, 0xd3 * 0x101};
    Q_DECL_UNUSED constexpr QColor lightgreen               {QColor::Rgb, 0xff * 0x101, 0x90 * 0x101, 0xee * 0x101, 0x90 * 0x101};
    Q_DECL_UNUSED constexpr QColor lightgrey                {QColor::Rgb, 0xff * 0x101, 0xd3 * 0x101, 0xd3 * 0x101, 0xd3 * 0x101};
    Q_DECL_UNUSED constexpr QColor lightpink                {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xb6 * 0x101, 0xc1 * 0x101};
    Q_DECL_UNUSED constexpr QColor lightsalmon              {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xa0 * 0x101, 0x7a * 0x101};
    Q_DECL_UNUSED constexpr QColor lightseagreen            {QColor::Rgb, 0xff * 0x101, 0x20 * 0x101, 0xb2 * 0x101, 0xaa * 0x101};
    Q_DECL_UNUSED constexpr QColor lightskyblue             {QColor::Rgb, 0xff * 0x101, 0x87 * 0x101, 0xce * 0x101, 0xfa * 0x101};
    Q_DECL_UNUSED constexpr QColor lightslategray           {QColor::Rgb, 0xff * 0x101, 0x77 * 0x101, 0x88 * 0x101, 0x99 * 0x101};
    Q_DECL_UNUSED constexpr QColor lightslategrey           {QColor::Rgb, 0xff * 0x101, 0x77 * 0x101, 0x88 * 0x101, 0x99 * 0x101};
    Q_DECL_UNUSED constexpr QColor lightsteelblue           {QColor::Rgb, 0xff * 0x101, 0xb0 * 0x101, 0xc4 * 0x101, 0xde * 0x101};
    Q_DECL_UNUSED constexpr QColor lightyellow              {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xff * 0x101, 0xe0 * 0x101};
    Q_DECL_UNUSED constexpr QColor lime                     {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0xff * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor limegreen                {QColor::Rgb, 0xff * 0x101, 0x32 * 0x101, 0xcd * 0x101, 0x32 * 0x101};
    Q_DECL_UNUSED constexpr QColor linen                    {QColor::Rgb, 0xff * 0x101, 0xfa * 0x101, 0xf0 * 0x101, 0xe6 * 0x101};
    Q_DECL_UNUSED constexpr QColor magenta                  {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0x00 * 0x101, 0xff * 0x101};
    Q_DECL_UNUSED constexpr QColor maroon                   {QColor::Rgb, 0xff * 0x101, 0x80 * 0x101, 0x00 * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor mediumaquamarine         {QColor::Rgb, 0xff * 0x101, 0x66 * 0x101, 0xcd * 0x101, 0xaa * 0x101};
    Q_DECL_UNUSED constexpr QColor mediumblue               {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0x00 * 0x101, 0xcd * 0x101};
    Q_DECL_UNUSED constexpr QColor mediumorchid             {QColor::Rgb, 0xff * 0x101, 0xba * 0x101, 0x55 * 0x101, 0xd3 * 0x101};
    Q_DECL_UNUSED constexpr QColor mediumpurple             {QColor::Rgb, 0xff * 0x101, 0x93 * 0x101, 0x70 * 0x101, 0xdb * 0x101};
    Q_DECL_UNUSED constexpr QColor mediumseagreen           {QColor::Rgb, 0xff * 0x101, 0x3c * 0x101, 0xb3 * 0x101, 0x71 * 0x101};
    Q_DECL_UNUSED constexpr QColor mediumslateblue          {QColor::Rgb, 0xff * 0x101, 0x7b * 0x101, 0x68 * 0x101, 0xee * 0x101};
    Q_DECL_UNUSED constexpr QColor mediumspringgreen        {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0xfa * 0x101, 0x9a * 0x101};
    Q_DECL_UNUSED constexpr QColor mediumturquoise          {QColor::Rgb, 0xff * 0x101, 0x48 * 0x101, 0xd1 * 0x101, 0xcc * 0x101};
    Q_DECL_UNUSED constexpr QColor mediumvioletred          {QColor::Rgb, 0xff * 0x101, 0xc7 * 0x101, 0x15 * 0x101, 0x85 * 0x101};
    Q_DECL_UNUSED constexpr QColor midnightblue             {QColor::Rgb, 0xff * 0x101, 0x19 * 0x101, 0x19 * 0x101, 0x70 * 0x101};
    Q_DECL_UNUSED constexpr QColor mintcream                {QColor::Rgb, 0xff * 0x101, 0xf5 * 0x101, 0xff * 0x101, 0xfa * 0x101};
    Q_DECL_UNUSED constexpr QColor mistyrose                {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xe4 * 0x101, 0xe1 * 0x101};
    Q_DECL_UNUSED constexpr QColor moccasin                 {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xe4 * 0x101, 0xb5 * 0x101};
    Q_DECL_UNUSED constexpr QColor navajowhite              {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xde * 0x101, 0xad * 0x101};
    Q_DECL_UNUSED constexpr QColor navy                     {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0x00 * 0x101, 0x80 * 0x101};
    Q_DECL_UNUSED constexpr QColor oldlace                  {QColor::Rgb, 0xff * 0x101, 0xfd * 0x101, 0xf5 * 0x101, 0xe6 * 0x101};
    Q_DECL_UNUSED constexpr QColor olive                    {QColor::Rgb, 0xff * 0x101, 0x80 * 0x101, 0x80 * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor olivedrab                {QColor::Rgb, 0xff * 0x101, 0x6b * 0x101, 0x8e * 0x101, 0x23 * 0x101};
    Q_DECL_UNUSED constexpr QColor orange                   {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xa5 * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor orangered                {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0x45 * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor orchid                   {QColor::Rgb, 0xff * 0x101, 0xda * 0x101, 0x70 * 0x101, 0xd6 * 0x101};
    Q_DECL_UNUSED constexpr QColor palegoldenrod            {QColor::Rgb, 0xff * 0x101, 0xee * 0x101, 0xe8 * 0x101, 0xaa * 0x101};
    Q_DECL_UNUSED constexpr QColor palegreen                {QColor::Rgb, 0xff * 0x101, 0x98 * 0x101, 0xfb * 0x101, 0x98 * 0x101};
    Q_DECL_UNUSED constexpr QColor paleturquoise            {QColor::Rgb, 0xff * 0x101, 0xaf * 0x101, 0xee * 0x101, 0xee * 0x101};
    Q_DECL_UNUSED constexpr QColor palevioletred            {QColor::Rgb, 0xff * 0x101, 0xdb * 0x101, 0x70 * 0x101, 0x93 * 0x101};
    Q_DECL_UNUSED constexpr QColor papayawhip               {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xef * 0x101, 0xd5 * 0x101};
    Q_DECL_UNUSED constexpr QColor peachpuff                {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xda * 0x101, 0xb9 * 0x101};
    Q_DECL_UNUSED constexpr QColor peru                     {QColor::Rgb, 0xff * 0x101, 0xcd * 0x101, 0x85 * 0x101, 0x3f * 0x101};
    Q_DECL_UNUSED constexpr QColor pink                     {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xc0 * 0x101, 0xcb * 0x101};
    Q_DECL_UNUSED constexpr QColor plum                     {QColor::Rgb, 0xff * 0x101, 0xdd * 0x101, 0xa0 * 0x101, 0xdd * 0x101};
    Q_DECL_UNUSED constexpr QColor powderblue               {QColor::Rgb, 0xff * 0x101, 0xb0 * 0x101, 0xe0 * 0x101, 0xe6 * 0x101};
    Q_DECL_UNUSED constexpr QColor purple                   {QColor::Rgb, 0xff * 0x101, 0x80 * 0x101, 0x00 * 0x101, 0x80 * 0x101};
    Q_DECL_UNUSED constexpr QColor red                      {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0x00 * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor rosybrown                {QColor::Rgb, 0xff * 0x101, 0xbc * 0x101, 0x8f * 0x101, 0x8f * 0x101};
    Q_DECL_UNUSED constexpr QColor royalblue                {QColor::Rgb, 0xff * 0x101, 0x41 * 0x101, 0x69 * 0x101, 0xe1 * 0x101};
    Q_DECL_UNUSED constexpr QColor saddlebrown              {QColor::Rgb, 0xff * 0x101, 0x8b * 0x101, 0x45 * 0x101, 0x13 * 0x101};
    Q_DECL_UNUSED constexpr QColor salmon                   {QColor::Rgb, 0xff * 0x101, 0xfa * 0x101, 0x80 * 0x101, 0x72 * 0x101};
    Q_DECL_UNUSED constexpr QColor sandybrown               {QColor::Rgb, 0xff * 0x101, 0xf4 * 0x101, 0xa4 * 0x101, 0x60 * 0x101};
    Q_DECL_UNUSED constexpr QColor seagreen                 {QColor::Rgb, 0xff * 0x101, 0x2e * 0x101, 0x8b * 0x101, 0x57 * 0x101};
    Q_DECL_UNUSED constexpr QColor seashell                 {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xf5 * 0x101, 0xee * 0x101};
    Q_DECL_UNUSED constexpr QColor sienna                   {QColor::Rgb, 0xff * 0x101, 0xa0 * 0x101, 0x52 * 0x101, 0x2d * 0x101};
    Q_DECL_UNUSED constexpr QColor silver                   {QColor::Rgb, 0xff * 0x101, 0xc0 * 0x101, 0xc0 * 0x101, 0xc0 * 0x101};
    Q_DECL_UNUSED constexpr QColor skyblue                  {QColor::Rgb, 0xff * 0x101, 0x87 * 0x101, 0xce * 0x101, 0xeb * 0x101};
    Q_DECL_UNUSED constexpr QColor slateblue                {QColor::Rgb, 0xff * 0x101, 0x6a * 0x101, 0x5a * 0x101, 0xcd * 0x101};
    Q_DECL_UNUSED constexpr QColor slategray                {QColor::Rgb, 0xff * 0x101, 0x70 * 0x101, 0x80 * 0x101, 0x90 * 0x101};
    Q_DECL_UNUSED constexpr QColor slategrey                {QColor::Rgb, 0xff * 0x101, 0x70 * 0x101, 0x80 * 0x101, 0x90 * 0x101};
    Q_DECL_UNUSED constexpr QColor snow                     {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xfa * 0x101, 0xfa * 0x101};
    Q_DECL_UNUSED constexpr QColor springgreen              {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0xff * 0x101, 0x7f * 0x101};
    Q_DECL_UNUSED constexpr QColor steelblue                {QColor::Rgb, 0xff * 0x101, 0x46 * 0x101, 0x82 * 0x101, 0xb4 * 0x101};
    Q_DECL_UNUSED constexpr QColor tan                      {QColor::Rgb, 0xff * 0x101, 0xd2 * 0x101, 0xb4 * 0x101, 0x8c * 0x101};
    Q_DECL_UNUSED constexpr QColor teal                     {QColor::Rgb, 0xff * 0x101, 0x00 * 0x101, 0x80 * 0x101, 0x80 * 0x101};
    Q_DECL_UNUSED constexpr QColor thistle                  {QColor::Rgb, 0xff * 0x101, 0xd8 * 0x101, 0xbf * 0x101, 0xd8 * 0x101};
    Q_DECL_UNUSED constexpr QColor tomato                   {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0x63 * 0x101, 0x47 * 0x101};
    Q_DECL_UNUSED constexpr QColor turquoise                {QColor::Rgb, 0xff * 0x101, 0x40 * 0x101, 0xe0 * 0x101, 0xd0 * 0x101};
    Q_DECL_UNUSED constexpr QColor violet                   {QColor::Rgb, 0xff * 0x101, 0xee * 0x101, 0x82 * 0x101, 0xee * 0x101};
    Q_DECL_UNUSED constexpr QColor wheat                    {QColor::Rgb, 0xff * 0x101, 0xf5 * 0x101, 0xde * 0x101, 0xb3 * 0x101};
    Q_DECL_UNUSED constexpr QColor white                    {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xff * 0x101, 0xff * 0x101};
    Q_DECL_UNUSED constexpr QColor whitesmoke               {QColor::Rgb, 0xff * 0x101, 0xf5 * 0x101, 0xf5 * 0x101, 0xf5 * 0x101};
    Q_DECL_UNUSED constexpr QColor yellow                   {QColor::Rgb, 0xff * 0x101, 0xff * 0x101, 0xff * 0x101, 0x00 * 0x101};
    Q_DECL_UNUSED constexpr QColor yellowgreen              {QColor::Rgb, 0xff * 0x101, 0x9a * 0x101, 0xcd * 0x101, 0x32 * 0x101};
}  // namespace Svg
#endif // Q_COMPILER_CONSTEXPR && Q_COMPILER_UNIFORM_INIT
}  // namespace QColorLiterals

QT_END_NAMESPACE

#endif // QCOLOR_H
