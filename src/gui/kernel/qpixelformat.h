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
    // QPixelFormat basically is a glorified quint64, split into several fields.
    // We could use bit-fields, but GCC at least generates horrible, horrible code for them,
    // so we do the bit-twiddling ourselves.
    enum FieldWidth {
        ModelFieldWidth = 4,
        FirstFieldWidth = 6,
        SecondFieldWidth = FirstFieldWidth,
        ThirdFieldWidth = FirstFieldWidth,
        FourthFieldWidth = FirstFieldWidth,
        FifthFieldWidth = FirstFieldWidth,
        AlphaFieldWidth = FirstFieldWidth,
        AlphaUsageFieldWidth = 1,
        AlphaPositionFieldWidth = 1,
        PremulFieldWidth = 1,
        TypeInterpretationFieldWidth = 4,
        ByteOrderFieldWidth = 2,
        SubEnumFieldWidth = 6,
        UnusedFieldWidth = 9,

        TotalFieldWidthByWidths = ModelFieldWidth + FirstFieldWidth + SecondFieldWidth + ThirdFieldWidth +
                                  FourthFieldWidth + FifthFieldWidth + AlphaFieldWidth + AlphaUsageFieldWidth +
                                  AlphaPositionFieldWidth + PremulFieldWidth + TypeInterpretationFieldWidth +
                                  ByteOrderFieldWidth + SubEnumFieldWidth + UnusedFieldWidth
    };

    enum Field {
        ModelField = 0,
        FirstField = ModelField + ModelFieldWidth,
        SecondField = FirstField + FirstFieldWidth,
        ThirdField = SecondField + SecondFieldWidth,
        FourthField = ThirdField + ThirdFieldWidth,
        FifthField = FourthField + FourthFieldWidth,
        AlphaField = FifthField + FifthFieldWidth,
        AlphaUsageField = AlphaField + AlphaFieldWidth,
        AlphaPositionField = AlphaUsageField + AlphaUsageFieldWidth,
        PremulField = AlphaPositionField + AlphaPositionFieldWidth,
        TypeInterpretationField = PremulField + PremulFieldWidth,
        ByteOrderField = TypeInterpretationField + TypeInterpretationFieldWidth,
        SubEnumField = ByteOrderField + ByteOrderFieldWidth,
        UnusedField = SubEnumField + SubEnumFieldWidth,

        TotalFieldWidthByOffsets = UnusedField + UnusedFieldWidth
    };

    Q_STATIC_ASSERT(uint(TotalFieldWidthByWidths) == uint(TotalFieldWidthByOffsets));
    Q_STATIC_ASSERT(uint(TotalFieldWidthByWidths) == 8 * sizeof(quint64));

    Q_DECL_CONSTEXPR inline uchar get(Field offset, FieldWidth width) const Q_DECL_NOTHROW
    { return uchar((data >> uint(offset)) & ((Q_UINT64_C(1) << uint(width)) - Q_UINT64_C(1))); }
    Q_DECL_CONSTEXPR static inline quint64 set(Field offset, FieldWidth width, uchar value)
    { return (quint64(value) & ((Q_UINT64_C(1) << uint(width)) - Q_UINT64_C(1))) << uint(offset); }

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

    Q_DECL_CONSTEXPR inline ColorModel colorModel() const  Q_DECL_NOTHROW { return ColorModel(get(ModelField, ModelFieldWidth)); }
    Q_DECL_CONSTEXPR inline uchar channelCount() const Q_DECL_NOTHROW { return (get(FirstField, FirstFieldWidth) > 0) +
                                                                                 (get(SecondField, SecondFieldWidth) > 0) +
                                                                                 (get(ThirdField, ThirdFieldWidth) > 0) +
                                                                                 (get(FourthField, FourthFieldWidth) > 0) +
                                                                                 (get(FifthField, FifthFieldWidth) > 0) +
                                                                                 (get(AlphaField, AlphaFieldWidth) > 0); }

    Q_DECL_CONSTEXPR inline uchar redSize() const Q_DECL_NOTHROW { return get(FirstField, FirstFieldWidth); }
    Q_DECL_CONSTEXPR inline uchar greenSize() const Q_DECL_NOTHROW { return get(SecondField, SecondFieldWidth); }
    Q_DECL_CONSTEXPR inline uchar blueSize() const Q_DECL_NOTHROW { return get(ThirdField, ThirdFieldWidth); }

    Q_DECL_CONSTEXPR inline uchar cyanSize() const Q_DECL_NOTHROW { return get(FirstField, FirstFieldWidth); }
    Q_DECL_CONSTEXPR inline uchar magentaSize() const Q_DECL_NOTHROW { return get(SecondField, SecondFieldWidth); }
    Q_DECL_CONSTEXPR inline uchar yellowSize() const Q_DECL_NOTHROW { return get(ThirdField, ThirdFieldWidth); }
    Q_DECL_CONSTEXPR inline uchar blackSize() const Q_DECL_NOTHROW { return get(FourthField, FourthFieldWidth); }

    Q_DECL_CONSTEXPR inline uchar hueSize() const Q_DECL_NOTHROW { return get(FirstField, FirstFieldWidth); }
    Q_DECL_CONSTEXPR inline uchar saturationSize() const Q_DECL_NOTHROW { return get(SecondField, SecondFieldWidth); }
    Q_DECL_CONSTEXPR inline uchar lightnessSize() const Q_DECL_NOTHROW { return get(ThirdField, ThirdFieldWidth); }
    Q_DECL_CONSTEXPR inline uchar brightnessSize() const Q_DECL_NOTHROW { return get(ThirdField, ThirdFieldWidth); }

    Q_DECL_CONSTEXPR inline uchar alphaSize() const Q_DECL_NOTHROW { return get(AlphaField, AlphaFieldWidth); }

    Q_DECL_CONSTEXPR inline uchar bitsPerPixel() const Q_DECL_NOTHROW { return get(FirstField, FirstFieldWidth) +
                                                                                 get(SecondField, SecondFieldWidth) +
                                                                                 get(ThirdField, ThirdFieldWidth) +
                                                                                 get(FourthField, FourthFieldWidth) +
                                                                                 get(FifthField, FifthFieldWidth) +
                                                                                 get(AlphaField, AlphaFieldWidth); }

    Q_DECL_CONSTEXPR inline AlphaUsage alphaUsage() const Q_DECL_NOTHROW { return AlphaUsage(get(AlphaUsageField, AlphaUsageFieldWidth)); }
    Q_DECL_CONSTEXPR inline AlphaPosition alphaPosition() const Q_DECL_NOTHROW { return AlphaPosition(get(AlphaPositionField, AlphaPositionFieldWidth)); }
    Q_DECL_CONSTEXPR inline AlphaPremultiplied premultiplied() const Q_DECL_NOTHROW { return AlphaPremultiplied(get(PremulField, PremulFieldWidth)); }
    Q_DECL_CONSTEXPR inline TypeInterpretation typeInterpretation() const Q_DECL_NOTHROW { return TypeInterpretation(get(TypeInterpretationField, TypeInterpretationFieldWidth)); }
    Q_DECL_CONSTEXPR inline ByteOrder byteOrder() const Q_DECL_NOTHROW { return ByteOrder(get(ByteOrderField, ByteOrderFieldWidth)); }

    Q_DECL_CONSTEXPR inline YUVLayout yuvLayout() const Q_DECL_NOTHROW { return YUVLayout(get(SubEnumField, SubEnumFieldWidth)); }
    Q_DECL_CONSTEXPR inline uchar subEnum() const Q_DECL_NOTHROW { return get(SubEnumField, SubEnumFieldWidth); }

private:
    Q_DECL_CONSTEXPR static inline ByteOrder resolveByteOrder(ByteOrder bo)
    { return bo == CurrentSystemEndian ? Q_BYTE_ORDER == Q_LITTLE_ENDIAN ? LittleEndian : BigEndian : bo ; }

private:
    quint64 data;

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
    : data(0)
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
    : data(set(ModelField, ModelFieldWidth, uchar(mdl)) |
           set(FirstField, FirstFieldWidth, firstSize) |
           set(SecondField, SecondFieldWidth, secondSize) |
           set(ThirdField, ThirdFieldWidth, thirdSize) |
           set(FourthField, FourthFieldWidth, fourthSize) |
           set(FifthField, FifthFieldWidth, fifthSize) |
           set(AlphaField, AlphaFieldWidth, alfa) |
           set(AlphaUsageField, AlphaUsageFieldWidth, uchar(usage)) |
           set(AlphaPositionField, AlphaPositionFieldWidth, uchar(position)) |
           set(PremulField, PremulFieldWidth, uchar(premult)) |
           set(TypeInterpretationField, TypeInterpretationFieldWidth, uchar(typeInterp)) |
           set(ByteOrderField, ByteOrderFieldWidth, uchar(resolveByteOrder(b_order))) |
           set(SubEnumField, SubEnumFieldWidth, s_enum) |
           set(UnusedField, UnusedFieldWidth, 0))
{
}

Q_DECL_CONST_FUNCTION Q_DECL_CONSTEXPR inline bool operator==(const QPixelFormat &fmt1, const QPixelFormat &fmt2)
{ return fmt1.data == fmt2.data; }

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
