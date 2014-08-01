/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPIXELFORMAT_H
#define QPIXELFORMAT_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

class QPixelFormat
{
public:
    enum ColorModel {
        RGB,
        BGR,
        Indexed,
        Grayscale,
        CMYK,
        HSL,
        HSV,
        YUV
    };

    enum AlphaUsage {
        UsesAlpha,
        IgnoresAlpha
    };

    enum AlphaPosition {
        AtBeginning,
        AtEnd
    };

    enum AlphaPremultiplied {
        NotPremultiplied,
        Premultiplied
    };

    enum TypeInterpretation {
        UnsignedInteger,
        UnsignedShort,
        UnsignedByte,
        FloatingPoint
    };

    enum YUVLayout {
        YUV444,
        YUV422,
        YUV411,
        YUV420P,
        YUV420SP,
        YV12,
        UYVY,
        YUYV,
        NV12,
        NV21,
        IMC1,
        IMC2,
        IMC3,
        IMC4,
        Y8,
        Y16
    };

    enum ByteOrder {
        LittleEndian,
        BigEndian,
        CurrentSystemEndian
    };

    Q_DECL_CONSTEXPR inline QPixelFormat() Q_DECL_NOTHROW;
    Q_DECL_CONSTEXPR inline QPixelFormat(ColorModel colorModel,
                                           uchar firstSize,
                                           uchar secondSize,
                                           uchar thirdSize,
                                           uchar fourthSize,
                                           uchar fifthSize,
                                           uchar alphaSize,
                                           AlphaUsage alphaUsage,
                                           AlphaPosition alphaPosition,
                                           AlphaPremultiplied premultiplied,
                                           TypeInterpretation typeInterpretation,
                                           ByteOrder byteOrder = CurrentSystemEndian,
                                           uchar subEnum = 0) Q_DECL_NOTHROW;

    Q_DECL_CONSTEXPR inline ColorModel colorModel() const  Q_DECL_NOTHROW { return ColorModel(model); }
    Q_DECL_CONSTEXPR inline uchar channelCount() const Q_DECL_NOTHROW { return (first > 0) +
                                                                                 (second > 0) +
                                                                                 (third > 0) +
                                                                                 (fourth > 0) +
                                                                                 (fifth > 0) +
                                                                                 (alpha > 0); }

    Q_DECL_CONSTEXPR inline uchar redSize() const Q_DECL_NOTHROW { return first; }
    Q_DECL_CONSTEXPR inline uchar greenSize() const Q_DECL_NOTHROW { return second; }
    Q_DECL_CONSTEXPR inline uchar blueSize() const Q_DECL_NOTHROW { return third; }

    Q_DECL_CONSTEXPR inline uchar cyanSize() const Q_DECL_NOTHROW { return first; }
    Q_DECL_CONSTEXPR inline uchar magentaSize() const Q_DECL_NOTHROW { return second; }
    Q_DECL_CONSTEXPR inline uchar yellowSize() const Q_DECL_NOTHROW { return third; }
    Q_DECL_CONSTEXPR inline uchar blackSize() const Q_DECL_NOTHROW { return fourth; }

    Q_DECL_CONSTEXPR inline uchar hueSize() const Q_DECL_NOTHROW { return first; }
    Q_DECL_CONSTEXPR inline uchar saturationSize() const Q_DECL_NOTHROW { return second; }
    Q_DECL_CONSTEXPR inline uchar lightnessSize() const Q_DECL_NOTHROW { return third; }
    Q_DECL_CONSTEXPR inline uchar brightnessSize() const Q_DECL_NOTHROW { return third; }

    Q_DECL_CONSTEXPR inline uchar alphaSize() const Q_DECL_NOTHROW { return alpha; }

    Q_DECL_CONSTEXPR inline uchar bitsPerPixel() const Q_DECL_NOTHROW { return first +
                                                                                 second +
                                                                                 third +
                                                                                 fourth +
                                                                                 fifth +
                                                                                 alpha; }

    Q_DECL_CONSTEXPR inline AlphaUsage alphaUsage() const Q_DECL_NOTHROW { return AlphaUsage(alpha_usage); }
    Q_DECL_CONSTEXPR inline AlphaPosition alphaPosition() const Q_DECL_NOTHROW { return AlphaPosition(alpha_position); }
    Q_DECL_CONSTEXPR inline AlphaPremultiplied premultiplied() const Q_DECL_NOTHROW { return AlphaPremultiplied(premul); }
    Q_DECL_CONSTEXPR inline TypeInterpretation typeInterpretation() const Q_DECL_NOTHROW { return TypeInterpretation(type_interpretation); }
    Q_DECL_CONSTEXPR inline ByteOrder byteOrder() const Q_DECL_NOTHROW { return ByteOrder(byte_order); }

    Q_DECL_CONSTEXPR inline YUVLayout yuvLayout() const Q_DECL_NOTHROW { return YUVLayout(sub_enum); }
    Q_DECL_CONSTEXPR inline uchar subEnum() const Q_DECL_NOTHROW { return sub_enum; }

private:
    quint64 model : 4;
    quint64 first : 6;
    quint64 second : 6;
    quint64 third : 6;
    quint64 fourth : 6;
    quint64 fifth : 6;
    quint64 alpha : 6;
    quint64 alpha_usage : 1;
    quint64 alpha_position : 1;
    quint64 premul: 1;
    quint64 type_interpretation : 4;
    quint64 byte_order : 2;
    quint64 sub_enum : 6;
    quint64 unused : 9;

    friend Q_DECL_CONST_FUNCTION Q_DECL_CONSTEXPR inline bool operator==(const QPixelFormat &fmt1, const QPixelFormat &fmt2);
    friend Q_DECL_CONST_FUNCTION Q_DECL_CONSTEXPR inline bool operator!=(const QPixelFormat &fmt1, const QPixelFormat &fmt2);
};
Q_STATIC_ASSERT(sizeof(QPixelFormat) == sizeof(quint64));
Q_DECLARE_TYPEINFO(QPixelFormat, Q_PRIMITIVE_TYPE);

class QPixelFormatRgb : public QPixelFormat
{
public:
    Q_DECL_CONSTEXPR
    inline QPixelFormatRgb(uchar redSize,
                           uchar greenSize,
                           uchar blueSize,
                           uchar alphaSize,
                           AlphaUsage alphaUsage,
                           AlphaPosition alphaPosition,
                           AlphaPremultiplied premultiplied = NotPremultiplied,
                           TypeInterpretation typeInterpretation = UnsignedInteger) Q_DECL_NOTHROW;
};

class QPixelFormatGrayscale : public QPixelFormat
{
public:
    Q_DECL_CONSTEXPR
    inline QPixelFormatGrayscale(uchar bufferSize,
                                 TypeInterpretation typeInterpretation = UnsignedInteger) Q_DECL_NOTHROW;
};

class QPixelFormatCmyk : public QPixelFormat
{
public:
    Q_DECL_CONSTEXPR
    inline QPixelFormatCmyk(uchar channelSize,
                            uchar alphaSize = 0,
                            AlphaUsage alphaUsage = IgnoresAlpha,
                            AlphaPosition alphaPosition = AtBeginning,
                            TypeInterpretation typeInterpretation = UnsignedInteger) Q_DECL_NOTHROW;
};

class QPixelFormatHsl : public QPixelFormat
{
public:
    Q_DECL_CONSTEXPR
    inline QPixelFormatHsl(uchar channelSize,
                           uchar alphaSize = 0,
                           AlphaUsage alphaUsage = IgnoresAlpha,
                           AlphaPosition alphaPosition = AtBeginning,
                           TypeInterpretation typeInterpretation = FloatingPoint) Q_DECL_NOTHROW;
};

class QPixelFormatHsv : public QPixelFormat
{
public:
    Q_DECL_CONSTEXPR
    inline QPixelFormatHsv(uchar channelSize,
                           uchar alphaSize = 0,
                           AlphaUsage alphaUsage = IgnoresAlpha,
                           AlphaPosition alphaPosition = AtBeginning,
                           TypeInterpretation typeInterpretation = FloatingPoint) Q_DECL_NOTHROW;
};

namespace QtPrivate {
    QPixelFormat Q_GUI_EXPORT QPixelFormat_createYUV(QPixelFormat::YUVLayout yuvLayout,
                                                     uchar alphaSize,
                                                     QPixelFormat::AlphaUsage alphaUsage,
                                                     QPixelFormat::AlphaPosition alphaPosition,
                                                     QPixelFormat::AlphaPremultiplied premultiplied,
                                                     QPixelFormat::TypeInterpretation typeInterpretation,
                                                     QPixelFormat::ByteOrder byteOrder);
}

class QPixelFormatYuv : public QPixelFormat
{
public:
    inline QPixelFormatYuv(YUVLayout yuvLayout,
                           uchar alphaSize = 0,
                           AlphaUsage alphaUsage = IgnoresAlpha,
                           AlphaPosition alphaPosition = AtBeginning,
                           AlphaPremultiplied premultiplied = NotPremultiplied,
                           TypeInterpretation typeInterpretation = UnsignedByte,
                           ByteOrder byteOrder = LittleEndian);
};

Q_DECL_CONSTEXPR
QPixelFormat::QPixelFormat() Q_DECL_NOTHROW
    : model(0)
    , first(0)
    , second(0)
    , third(0)
    , fourth(0)
    , fifth(0)
    , alpha(0)
    , alpha_usage(0)
    , alpha_position(0)
    , premul(0)
    , type_interpretation(0)
    , byte_order(0)
    , sub_enum(0)
    , unused(0)
{
}

Q_DECL_CONSTEXPR
QPixelFormat::QPixelFormat(ColorModel mdl,
                           uchar firstSize,
                           uchar secondSize,
                           uchar thirdSize,
                           uchar fourthSize,
                           uchar fifthSize,
                           uchar alfa,
                           AlphaUsage usage,
                           AlphaPosition position,
                           AlphaPremultiplied premult,
                           TypeInterpretation typeInterp,
                           ByteOrder b_order,
                           uchar s_enum) Q_DECL_NOTHROW
    : model(mdl)
    , first(firstSize)
    , second(secondSize)
    , third(thirdSize)
    , fourth(fourthSize)
    , fifth(fifthSize)
    , alpha(alfa)
    , alpha_usage(usage)
    , alpha_position(position)
    , premul(premult)
    , type_interpretation(typeInterp)
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    , byte_order(b_order == CurrentSystemEndian ? LittleEndian : b_order)
#else
    , byte_order(b_order == CurrentSystemEndian ? BigEndian : b_order)
#endif
    , sub_enum(s_enum)
    , unused(0)
{
}

Q_DECL_CONST_FUNCTION Q_DECL_CONSTEXPR inline bool operator==(const QPixelFormat &fmt1, const QPixelFormat &fmt2)
{
    return fmt1.model               == fmt2.model
        && fmt1.first               == fmt2.first
        && fmt1.second              == fmt2.second
        && fmt1.third               == fmt2.third
        && fmt1.fourth              == fmt2.fourth
        && fmt1.fifth               == fmt2.fifth
        && fmt1.alpha               == fmt2.alpha
        && fmt1.alpha_usage         == fmt2.alpha_usage
        && fmt1.alpha_position      == fmt2.alpha_position
        && fmt1.premul              == fmt2.premul
        && fmt1.type_interpretation == fmt2.type_interpretation
        && fmt1.byte_order          == fmt2.byte_order
        && fmt1.sub_enum            == fmt2.sub_enum
        && fmt1.unused              == fmt2.unused;
}

Q_DECL_CONST_FUNCTION Q_DECL_CONSTEXPR inline bool operator!=(const QPixelFormat &fmt1, const QPixelFormat &fmt2)
{ return !(fmt1 == fmt2); }

Q_DECL_CONSTEXPR
QPixelFormatRgb::QPixelFormatRgb(uchar red,
                                 uchar green,
                                 uchar blue,
                                 uchar alfa,
                                 AlphaUsage usage,
                                 AlphaPosition position,
                                 AlphaPremultiplied pmul,
                                 TypeInterpretation typeInt) Q_DECL_NOTHROW
    : QPixelFormat(RGB,
                   red,
                   green,
                   blue,
                   0,
                   0,
                   alfa,
                   usage,
                   position,
                   pmul,
                   typeInt)
{ }

Q_DECL_CONSTEXPR
QPixelFormatGrayscale::QPixelFormatGrayscale(uchar channelSize,
                                             TypeInterpretation typeInt) Q_DECL_NOTHROW
    : QPixelFormat(Grayscale,
                   channelSize,
                   0,
                   0,
                   0,
                   0,
                   0,
                   IgnoresAlpha,
                   AtBeginning,
                   NotPremultiplied,
                   typeInt)
{ }

Q_DECL_CONSTEXPR
QPixelFormatCmyk::QPixelFormatCmyk(uchar channelSize,
                                   uchar alfa,
                                   AlphaUsage usage,
                                   AlphaPosition position,
                                   TypeInterpretation typeInt) Q_DECL_NOTHROW
    : QPixelFormat(CMYK,
                   channelSize,
                   channelSize,
                   channelSize,
                   channelSize,
                   0,
                   alfa,
                   usage,
                   position,
                   NotPremultiplied,
                   typeInt)
{ }

Q_DECL_CONSTEXPR
QPixelFormatHsl::QPixelFormatHsl(uchar channelSize,
                                 uchar alfa,
                                 AlphaUsage usage,
                                 AlphaPosition position,
                                 TypeInterpretation typeInt) Q_DECL_NOTHROW
    : QPixelFormat(HSL,
                   channelSize,
                   channelSize,
                   channelSize,
                   0,
                   0,
                   alfa,
                   usage,
                   position,
                   NotPremultiplied,
                   typeInt)
{ }

Q_DECL_CONSTEXPR
QPixelFormatHsv::QPixelFormatHsv(uchar channelSize,
                                 uchar alfa,
                                 AlphaUsage usage,
                                 AlphaPosition position,
                                 TypeInterpretation typeInt) Q_DECL_NOTHROW
    : QPixelFormat(HSV,
                   channelSize,
                   channelSize,
                   channelSize,
                   0,
                   0,
                   alfa,
                   usage,
                   position,
                   NotPremultiplied,
                   typeInt)
{ }

QPixelFormatYuv::QPixelFormatYuv(YUVLayout layout,
                                 uchar alfa,
                                 AlphaUsage usage,
                                 AlphaPosition position,
                                 AlphaPremultiplied p_mul,
                                 TypeInterpretation typeInt,
                                 ByteOrder b_order)
    : QPixelFormat(QtPrivate::QPixelFormat_createYUV(layout,
                                                     alfa,
                                                     usage,
                                                     position,
                                                     p_mul,
                                                     typeInt,
                                                     b_order))

{ }

QT_END_NAMESPACE

#endif //QPIXELFORMAT_H
