// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpixelformat.h"

QT_BEGIN_NAMESPACE

/*!
    \class QPixelFormat
    \inmodule QtGui
    \since 5.4
    \brief QPixelFormat is a class for describing different pixel
    layouts in graphics buffers.

    In Qt there is a often a need to represent the layout of the pixels in a
    graphics buffer. QPixelFormat can describe up to 5 color channels and 1 alpha
    channel, including details about how these channels are represented in memory
    individually and in relation to each other.

    The typeInterpretation() and byteOrder() determines how each pixel should be
    read/interpreted, while alphaSize(), alphaUsage(), alphaPosition(), and
    premultiplied() describes the position and properties of the possible alpha
    channel.

    There is no support for describing YUV's macro pixels. Instead a list of
    \l{QPixelFormat::YUVLayout}{YUV formats} is provided. When a QPixelFormat
    describes a YUV format, the bitsPerPixel() value is deduced from the YUV layout.
*/

/*!
    \enum QPixelFormat::ColorModel

    This enum describes the \l{colorModel()}{color model} of the pixel format.

    \value RGB The color model is RGB.

    \value BGR This is logically the opposite endian version of RGB. However,
    for ease of use it has its own model.

    \value Indexed The color model uses a color palette.

    \value Grayscale The color model is Grayscale.

    \value CMYK The color model is CMYK.

    \value HSL The color model is HSL.

    \value HSV The color model is HSV.

    \value YUV The color model is YUV.

    \value Alpha [since 5.5] There is no color model, only alpha is used.
*/

/*!
    \enum QPixelFormat::AlphaUsage

    This enum describes the \l{alphaUsage()}{alpha usage} of the pixel format.

    \value IgnoresAlpha The alpha channel is not used.

    \value UsesAlpha    The alpha channel is used.

    \sa alphaSize(), alphaPosition(), premultiplied()
*/

/*!
    \enum QPixelFormat::AlphaPosition

    This enum describes the \l{alphaPosition()}{alpha position} of the pixel format.

    \value AtBeginning The alpha channel will be put in front of the color
                       channels. E.g. ARGB.

    \value AtEnd       The alpha channel will be put in the back of the color
                       channels. E.g. RGBA.

    \sa alphaSize(), alphaUsage(), premultiplied()
*/

/*!
    \enum QPixelFormat::AlphaPremultiplied

    This enum describes whether the alpha channel of the pixel format is
    \l{premultiplied}{premultiplied} into the color channels or not.

    \value NotPremultiplied The alpha channel is not multiplied into the color channels.

    \value Premultiplied    The alpha channel is multiplied into the color channels.

    \sa alphaSize(), alphaUsage(), alphaPosition()
*/

/*!
    \enum QPixelFormat::TypeInterpretation

    This enum describes the \l{typeInterpretation()}{type interpretation} of the pixel format.

    \value UnsignedInteger  The pixels should be read as one or more \c{unsigned int}.
    \value UnsignedShort    The pixels should be read as one or more \c{unsigned short}.
    \value UnsignedByte     The pixels should be read as one or more \c{byte}.
    \value FloatingPoint    The pixels should be read as one or more floating point
                            numbers, with the concrete type defined by the color/alpha
                            channel, ie. \c{qfloat16} for 16-bit half-float formats and
                            \c{float} for 32-bit full-float formats.

    \sa byteOrder()
*/

/*!
    \enum QPixelFormat::ByteOrder

    This enum describes the \l{byteOrder()}{byte order} of the pixel format.

    \value LittleEndian        The byte order is little endian.
    \value BigEndian           The byte order is big endian.
    \value CurrentSystemEndian This enum will not be stored, but is converted in
                               the constructor to the endian enum that matches
                               the enum of the current system.

    \sa typeInterpretation()
*/

/*!
    \enum QPixelFormat::YUVLayout

    This enum describes the \l{yuvLayout()}{YUV layout} of the pixel format,
    given that it has a color model of QPixelFormat::YUV.

    \value YUV444
    \value YUV422
    \value YUV411
    \value YUV420P
    \value YUV420SP
    \value YV12
    \value UYVY
    \value YUYV
    \value NV12
    \value NV21
    \value IMC1
    \value IMC2
    \value IMC3
    \value IMC4
    \value Y8
    \value Y16
*/

/*!
    \fn QPixelFormat::QPixelFormat()

    Creates a null pixelformat. This format maps to QImage::Format_Invalid.
*/

/*!
    \fn QPixelFormat::QPixelFormat(ColorModel colorModel,
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
                                   uchar subEnum = 0)

    Creates a QPixelFormat which assigns its data to the attributes.
    \a colorModel will be put into a buffer which is 4 bits long.

    \a firstSize \a secondSize \a thirdSize \a fourthSize \a fifthSize \a
    alphaSize are all meant to represent the size of a channel. The channels will
    be used for different uses dependent on the \a colorModel. For RGB the
    firstSize will represent the Red channel. On CMYK it will represent the
    value of the Cyan channel.

    \a alphaUsage represents if the alpha channel is used or not.

    \a alphaPosition is the position of the alpha channel.

    \a premultiplied represents if the alpha channel is already multiplied with
    the color channels.

    \a typeInterpretation is how the pixel is interpreted.

    \a byteOrder represents the endianness of the pixelformat. This defaults to
    CurrentSystemEndian, which will be resolve to the system's endianness for
    non-byte-ordered formats, and QPixelFormat::BigEndian for QPixelFormat::UnsignedByte.

    \a subEnum is used for colorModels that have to store some extra
    information with supplying an extra enum. This is used by YUV to store the
    YUV type The default value is 0.

    \note BGR formats have their own color model, and should not be described
    by using the opposite endianness of an RGB format.
*/

/*!
    \fn QPixelFormat qPixelFormatRgba(uchar redSize,
                                      uchar greenSize,
                                      uchar blueSize,
                                      uchar alphaSize,
                                      QPixelFormat::AlphaUsage alphaUsage,
                                      QPixelFormat::AlphaPosition alphaPosition,
                                      QPixelFormat::AlphaPremultiplied premultiplied = QPixelFormat::NotPremultiplied,
                                      QPixelFormat::TypeInterpretation typeInterpretation = QPixelFormat::UnsignedInteger)
    \relates QPixelFormat

    Constructor function making an RGB pixelformat. \a redSize \a greenSize \a
    blueSize represent the size of each color channel. \a alphaSize describes
    the alpha channel size and its position is described with \a alphaPosition.
    \a alphaUsage is used to determine if the alpha channel is used or not.
    Setting the alpha channel size to 8 and alphaUsage to IgnoresAlpha is how
    it is possible to create a 32 bit format where the rgb channels only use 24
    bits combined. \a premultiplied \a typeInterpretation are
    accessible with accessors with the same name.

    \sa QPixelFormat::TypeInterpretation
*/

/*!
    \fn QPixelFormat qPixelFormatGrayscale(uchar channelSize,
                                           QPixelFormat::TypeInterpretation typeInterpretation = QPixelFormat::UnsignedInteger)
    \relates QPixelFormat

    Constructor function for creating a Grayscale format. Monochrome formats can be
    described by passing 1 to \a channelSize. Its also possible to define very
    accurate grayscale formats using doubles to describe each pixel by passing 8
    as \a channelSize and FloatingPoint as \a typeInterpretation.

    \sa QPixelFormat::TypeInterpretation
*/

/*!
    \fn QPixelFormat qPixelFormatAlpha(uchar channelSize,
                                           QPixelFormat::TypeInterpretation typeInterpretation = QPixelFormat::UnsignedInteger)
    \relates QPixelFormat
    \since 5.5

    Constructor function for creating an Alpha format. A mask format can be
    described by passing 1 to \a channelSize. Its also possible to define very
    accurate alpha formats using doubles to describe each pixel by passing 8
    as \a channelSize and FloatingPoint as \a typeInterpretation.

    \sa QPixelFormat::TypeInterpretation
*/


/*!
    \fn QPixelFormat qPixelFormatCmyk(uchar channelSize,
                                      uchar alphaSize = 0,
                                      QPixelFormat::AlphaUsage alphaUsage = QPixelFormat::IgnoresAlpha,
                                      QPixelFormat::AlphaPosition alphaPosition = QPixelFormat::AtBeginning,
                                      QPixelFormat::TypeInterpretation typeInterpretation = QPixelFormat::UnsignedInteger)
    \relates QPixelFormat

    Constructor function for creating CMYK formats. The channel count will be 4 or
    5 depending on if \a alphaSize is bigger than zero or not. The CMYK color
    channels will all be set to the value of \a channelSize.

    \a alphaUsage \a alphaPosition and \a typeInterpretation are all accessible with
    the accessors with the same name.

    \sa QPixelFormat::TypeInterpretation
*/

/*!
    \fn QPixelFormat qPixelFormatHsl(uchar channelSize,
                                     uchar alphaSize = 0,
                                     QPixelFormat::AlphaUsage alphaUsage = QPixelFormat::IgnoresAlpha,
                                     QPixelFormat::AlphaPosition alphaPosition = QPixelFormat::AtBeginning,
                                     QPixelFormat::TypeInterpretation typeInterpretation = QPixelFormat::FloatingPoint)
    \relates QPixelFormat

    Constructor function for creating HSL formats. The channel count will be 3 or 4
    depending on if \a alphaSize is bigger than 0.

    \a channelSize will set the hueSize saturationSize and lightnessSize to the same value.

    \a alphaUsage \a alphaPosition and \a typeInterpretation are all accessible with
    the accessors with the same name.
*/

/*!
    \fn QPixelFormat qPixelFormatHsv(uchar channelSize,
                                     uchar alphaSize = 0,
                                     QPixelFormat::AlphaUsage alphaUsage = QPixelFormat::IgnoresAlpha,
                                     QPixelFormat::AlphaPosition alphaPosition = QPixelFormat::AtBeginning,
                                     QPixelFormat::TypeInterpretation typeInterpretation = QPixelFormat::FloatingPoint)
    \relates QPixelFormat

    Constructor function for creating HSV formats. The channel count will be 3 or 4
    depending on if \a alphaSize is bigger than 0.

    \a channelSize will set the hueSize saturationSize and brightnessSize to the same value.

    \a alphaUsage \a alphaPosition and \a typeInterpretation are all accessible with
    the accessors with the same name.
*/

/*!
    \fn QPixelFormat qPixelFormatYuv(QPixelFormat::YUVLayout yuvLayout,
                                     uchar alphaSize = 0,
                                     QPixelFormat::AlphaUsage alphaUsage = QPixelFormat::IgnoresAlpha,
                                     QPixelFormat::AlphaPosition alphaPosition = QPixelFormat::AtBeginning,
                                     QPixelFormat::AlphaPremultiplied premultiplied = QPixelFormat::NotPremultiplied,
                                     QPixelFormat::TypeInterpretation typeInterpretation = QPixelFormat::UnsignedByte,
                                     QPixelFormat::ByteOrder byteOrder = QPixelFormat::BigEndian)
    \relates QPixelFormat

    Constructor function for creating a QPixelFormat describing a YUV format with
    \a yuvLayout.  \a alphaSize describes the size of a potential alpha channel
    and is position is described with \a alphaPosition. The "first" "second" ..
    "fifth" channels are all set to 0. \a alphaUsage \a premultiplied \a
    typeInterpretation and \a byteOrder will work as with other formats.
*/

/*!
    \fn ColorModel QPixelFormat::colorModel() const

    Accessor function for the color model.

    Note that for QPixelFormat::YUV the individual macro pixels can not be
    described. Instead a list of \l{QPixelFormat::YUVLayout}{YUV formats} is provided,
    and the bitsPerPixel() value is deduced from the YUV layout.
*/

/*!
    \fn uchar QPixelFormat::channelCount() const

    Accessor function for the channel count.

    The channel count represents channels (color and alpha) with a size > 0.
*/

/*!
    \fn uchar QPixelFormat::redSize() const

    Accessor function for the size of the red color channel.
*/

/*!
    \fn uchar QPixelFormat::greenSize() const

    Accessor function for the size of the green color channel.
*/

/*!
    \fn uchar QPixelFormat::blueSize() const

    Accessor function for the size of the blue color channel.
*/

/*!
    \fn uchar QPixelFormat::cyanSize() const

    Accessor function for the cyan color channel.
*/

/*!
    \fn uchar QPixelFormat::magentaSize() const

    Accessor function for the megenta color channel.
*/

/*!
    \fn uchar QPixelFormat::yellowSize() const

    Accessor function for the yellow color channel.
*/

/*!
    \fn uchar QPixelFormat::blackSize() const

    Accessor function for the black/key color channel.
*/

/*!
    \fn uchar QPixelFormat::hueSize() const

    Accessor function for the hue channel size.
*/

/*!
    \fn uchar QPixelFormat::saturationSize() const

    Accessor function for the saturation channel size.
*/

/*!
    \fn uchar QPixelFormat::lightnessSize() const

    Accessor function for the lightness channel size.
*/

/*!
    \fn uchar QPixelFormat::brightnessSize() const

    Accessor function for the brightness channel size.
*/

/*!
    \fn uchar QPixelFormat::alphaSize() const

    Accessor function for the alpha channel size.
*/

/*!
    \fn uchar QPixelFormat::bitsPerPixel() const

    Accessor function for the bits used per pixel. This function returns the
    sum of all the color channels + the size of the alpha channel.
*/

/*!
    \fn AlphaPremultiplied QPixelFormat::premultiplied() const

    Accessor function for the whether the alpha channel is multiplied
    in to the color channels.
*/

/*!
    \fn TypeInterpretation QPixelFormat::typeInterpretation() const

    The type interpretation determines how each pixel should be read.

    Each pixel is represented as one or more units of the given type,
    laid out sequentially in memory.

    \note The \l{byteOrder()}{byte order} of the pixel format and the
    endianness of the host system only affect the memory layout of each
    individual unit being read — \e{not} the relative ordering of the
    units.

    For example, QImage::Format_Mono has a \l{QImage::pixelFormat()}{pixel format}
    of 1 bits per pixel and a QPixelFormat::UnsignedByte type interpretation,
    which should be read as a single \c{byte}. Similarly, QImage::Format_RGB888
    has a \l{QImage::pixelFormat()}{pixel format} of 24 bits per pixel, and
    and a QPixelFormat::UnsignedByte type interpretation, which should be
    read as three consecutive \c{byte}s.

    Many of the QImage \l{QImage::Format}{formats} are 32-bit with a type
    interpretation of QPixelFormat::UnsignedInteger, which should be read
    as a single \c{unsigned int}.

    For QPixelFormat::FloatingPoint formats like QImage::Format_RGBA16FPx4
    or QImage::Format_RGBA32FPx4 the type is determined based on the size
    of the individual color/alpha channels, with \c{qfloat16} for 16-bit
    half-float formats and \c{float} for 32-bit full-float formats.

    \sa byteOrder()
*/

/*!
    \fn ByteOrder QPixelFormat::byteOrder() const

    The byte order of the pixel format determines the memory layout of
    the individual type units, as described by the typeInterpretation().

    This function will never return QPixelFormat::CurrentSystemEndian as this
    value is translated to the system's endian value in the constructor.

    For pixel formats with typeInterpretation() QPixelFormat::UnsignedByte this
    will typically be QPixelFormat::BigEndian, while other type interpretations
    will typically reflect the endianness of the current system.

    If the byte order of the pixel format matches the current system the
    individual type units can be read and manipulated using the same bit
    masks and operations, regardless of the host system endianness. For
    example, with QImage::Format_ARGB32, which has a QPixelFormat::UnsignedInteger
    type interpretation, the alpha can always be read by masking the
    \c{unsigned int} by \c{0xFF000000}, regardless of the host endianness.

    If the pixel format and host endianness does \e{not} match care must
    be taken to account for this. Classes like QImage do not swap the
    internal bits to match the host system endianness in these cases.

    \sa typeInterpretation(), alphaPosition()
*/

/*!
    \fn AlphaUsage QPixelFormat::alphaUsage() const

    Accessor function for whether the alpha channel is used or not.

    Sometimes the pixel format reserves place for an alpha channel,
    so alphaSize() will return > 0, but the alpha channel is not
    used/ignored.

    For example, for QImage::Format_RGB32, the bitsPerPixel() is 32,
    because the alpha channel has a size of 8, but alphaUsage()
    reflects QPixelFormat::IgnoresAlpha.

    Note that in such situations the \l{alphaPosition()}{position} of
    the unused alpha channel is still important, as it affects the
    placement of the color channels.

    \sa alphaPosition(), alphaSize(), premultiplied()
*/

/*!
    \fn AlphaPosition QPixelFormat::alphaPosition() const

    Accessor function for the position of the alpha channel
    relative to the color channels.

    For formats where the individual channels map to individual
    units, the alpha position is relative to these units. For
    example for QImage::Format_RGBA16FPx4 which has an alpha
    position of QPixelFormat::AtEnd, the alpha is the last
    \c{qfloat16} read.

    For formats where multiple channels are packed into a single unit,
    the QPixelFormat::AtBeginning and QPixelFormat::AtEnd values map to
    the most significant and least significant bits of the packed unit,
    with respect to the format's own byteOrder().

    For example, for QImage::Format_ARGB32, which has a type interpretation
    of QPixelFormat::UnsignedInteger and a byteOrder() that always matches
    the host system, the alpha position of QPixelFormat::AtBeginning means
    that the alpha can always be found at \c{0xFF000000}.

    If the pixel format and host endianness does \e{not} match care must be
    taken to correctly map the pixel format layout to the host memory layout.

    \sa alphaUsage(), alphaSize(), premultiplied()
*/

/*!
    \fn YUVLayout QPixelFormat::yuvLayout() const

    Accessor function for the YUVLayout. It is difficult to describe the color
    channels of a YUV pixel format since YUV color model uses macro pixels.
    Instead the layout of the pixels are stored as an enum.
*/

/*!
    \fn uchar QPixelFormat::subEnum() const

    Accessor for the datapart which contains subEnums
    This is the same as the yuvLayout() function.

    \sa yuvLayout()
    \internal
*/

static_assert(sizeof(QPixelFormat) == sizeof(quint64));


namespace QtPrivate {
    QPixelFormat QPixelFormat_createYUV(QPixelFormat::YUVLayout yuvLayout,
                                        uchar alphaSize,
                                        QPixelFormat::AlphaUsage alphaUsage,
                                        QPixelFormat::AlphaPosition alphaPosition,
                                        QPixelFormat::AlphaPremultiplied premultiplied,
                                        QPixelFormat::TypeInterpretation typeInterpretation,
                                        QPixelFormat::ByteOrder byteOrder)
    {
        uchar bits_per_pixel = 0;
        switch (yuvLayout) {
        case QPixelFormat::YUV444:
            bits_per_pixel = 24;
            break;
        case QPixelFormat::YUV422:
            bits_per_pixel = 16;
            break;
        case QPixelFormat::YUV411:
        case QPixelFormat::YUV420P:
        case QPixelFormat::YUV420SP:
        case QPixelFormat::YV12:
            bits_per_pixel = 12;
            break;
        case QPixelFormat::UYVY:
        case QPixelFormat::YUYV:
            bits_per_pixel = 16;
            break;
        case QPixelFormat::NV12:
        case QPixelFormat::NV21:
            bits_per_pixel = 12;
            break;
        case QPixelFormat::IMC1:
        case QPixelFormat::IMC2:
        case QPixelFormat::IMC3:
        case QPixelFormat::IMC4:
            bits_per_pixel = 12;
            break;
        case QPixelFormat::Y8:
            bits_per_pixel = 8;
            break;
        case QPixelFormat::Y16:
            bits_per_pixel = 16;
            break;
        }

        return QPixelFormat(QPixelFormat::YUV,
                            0, 0, 0, 0,
                            bits_per_pixel,
                            alphaSize,
                            alphaUsage,
                            alphaPosition,
                            premultiplied,
                            typeInterpretation,
                            byteOrder,
                            yuvLayout);
    }
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QPixelFormat &f)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg.noquote();
    dbg.verbosity(0);

    dbg << "QPixelFormat(" << f.colorModel();
    if (f.colorModel() == QPixelFormat::YUV)
        dbg << "," << f.yuvLayout();

    dbg << ",bpp=" << f.bitsPerPixel()
        << "," << f.typeInterpretation();

    if (f.typeInterpretation() != QPixelFormat::UnsignedByte || f.bitsPerPixel() > 8)
        dbg << "," << f.byteOrder();

    if (f.colorModel() != QPixelFormat::YUV) {
        dbg << ",ch=" << f.channelCount() << "[";
        const int alphaSize = f.alphaSize();
        const int colorChannels = f.channelCount() - (f.alphaSize() ? 1 : 0);
        if (alphaSize && f.alphaPosition() == QPixelFormat::AtBeginning)
            dbg << alphaSize << "-";
        if (colorChannels >= 1)
            dbg << f.get(QPixelFormat::FirstField, QPixelFormat::FirstFieldWidth);
        if (colorChannels >= 2)
            dbg << "-" << f.get(QPixelFormat::SecondField, QPixelFormat::SecondFieldWidth);
        if (colorChannels >= 3)
            dbg << "-" << f.get(QPixelFormat::ThirdField, QPixelFormat::ThirdFieldWidth);
        if (colorChannels >= 4)
            dbg << "-" << f.get(QPixelFormat::FourthField, QPixelFormat::FourthFieldWidth);
        if (colorChannels >= 5)
            dbg << "-" << f.get(QPixelFormat::FifthField, QPixelFormat::FifthFieldWidth);
        if (alphaSize && f.alphaPosition() == QPixelFormat::AtEnd)
            dbg << "-" << alphaSize;
        dbg << "]";
    }

    if (f.alphaSize() > 0) {
        dbg << "," << f.alphaUsage()
            << "=" << f.alphaSize()
            << "," << f.alphaPosition();
        if (f.alphaUsage() == QPixelFormat::UsesAlpha)
            dbg << "," << f.premultiplied();
    } else {
        dbg << ",NoAlpha";
    }

    dbg << ')';
    return dbg;
}
#endif

QT_END_NAMESPACE

#include "qpixelformat.moc"
