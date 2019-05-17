/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "qcolorspace.h"
#include "qcolorspace_p.h"

#include "qcolortransform.h"
#include "qcolormatrix_p.h"
#include "qcolortransferfunction_p.h"
#include "qcolortransform_p.h"
#include "qicc_p.h"

#include <qmath.h>
#include <qtransform.h>

#include <qdebug.h>

QT_BEGIN_NAMESPACE

QBasicMutex QColorSpacePrivate::s_lutWriteLock;

QColorSpacePrimaries::QColorSpacePrimaries(QColorSpace::Gamut gamut)
{
    switch (gamut) {
    case QColorSpace::Gamut::SRgb:
        redPoint   = QPointF(0.640, 0.330);
        greenPoint = QPointF(0.300, 0.600);
        bluePoint  = QPointF(0.150, 0.060);
        whitePoint = QColorVector::D65Chromaticity();
        break;
    case QColorSpace::Gamut::DciP3D65:
        redPoint   = QPointF(0.680, 0.320);
        greenPoint = QPointF(0.265, 0.690);
        bluePoint  = QPointF(0.150, 0.060);
        whitePoint = QColorVector::D65Chromaticity();
        break;
    case QColorSpace::Gamut::Bt2020:
        redPoint   = QPointF(0.708, 0.292);
        greenPoint = QPointF(0.190, 0.797);
        bluePoint  = QPointF(0.131, 0.046);
        whitePoint = QColorVector::D65Chromaticity();
        break;
    case QColorSpace::Gamut::AdobeRgb:
        redPoint   = QPointF(0.640, 0.330);
        greenPoint = QPointF(0.210, 0.710);
        bluePoint  = QPointF(0.150, 0.060);
        whitePoint = QColorVector::D65Chromaticity();
        break;
    case QColorSpace::Gamut::ProPhotoRgb:
        redPoint   = QPointF(0.7347, 0.2653);
        greenPoint = QPointF(0.1596, 0.8404);
        bluePoint  = QPointF(0.0366, 0.0001);
        whitePoint = QColorVector::D50Chromaticity();
        break;
    default:
        Q_UNREACHABLE();
    }
}

bool QColorSpacePrimaries::areValid() const
{
    if (!QColorVector::isValidChromaticity(redPoint))
        return false;
    if (!QColorVector::isValidChromaticity(greenPoint))
        return false;
    if (!QColorVector::isValidChromaticity(bluePoint))
        return false;
    if (!QColorVector::isValidChromaticity(whitePoint))
        return false;
    return true;
}

QColorMatrix QColorSpacePrimaries::toXyzMatrix() const
{
    // This converts to XYZ in some undefined scale.
    QColorMatrix toXyz = { QColorVector(redPoint),
                           QColorVector(greenPoint),
                           QColorVector(bluePoint) };

    // Since the white point should be (1.0, 1.0, 1.0) in the
    // input, we can figure out the scale by using the
    // inverse conversion on the white point.
    QColorVector wXyz(whitePoint);
    QColorVector whiteScale = toXyz.inverted().map(wXyz);

    // Now we have scaled conversion to XYZ relative to the given whitepoint
    toXyz = toXyz * QColorMatrix::fromScale(whiteScale);

    // But we want a conversion to XYZ relative to D50
    QColorVector wXyzD50 = QColorVector::D50();

    if (wXyz != wXyzD50) {
        // Do chromatic adaptation to map our white point to XYZ D50.

        // The Bradford method chromatic adaptation matrix:
        QColorMatrix abrad = { {  0.8951f, -0.7502f,  0.0389f },
                               {  0.2664f,  1.7135f, -0.0685f },
                               { -0.1614f,  0.0367f,  1.0296f } };
        QColorMatrix abradinv = { {  0.9869929f, 0.4323053f, -0.0085287f },
                                  { -0.1470543f, 0.5183603f,  0.0400428f },
                                  {  0.1599627f, 0.0492912f,  0.9684867f } };

        QColorVector srcCone = abrad.map(wXyz);
        QColorVector dstCone = abrad.map(wXyzD50);

        QColorMatrix wToD50 = { { dstCone.x / srcCone.x, 0, 0 },
                                { 0, dstCone.y / srcCone.y, 0 },
                                { 0, 0, dstCone.z / srcCone.z } };


        QColorMatrix chromaticAdaptation = abradinv * (wToD50 * abrad);
        toXyz = chromaticAdaptation * toXyz;
    }

    return toXyz;
}

QColorSpacePrivate::QColorSpacePrivate()
        : id(QColorSpace::Unknown)
        , gamut(QColorSpace::Gamut::Custom)
        , transferFunction(QColorSpace::TransferFunction::Custom)
        , gamma(0.0f)
        , whitePoint(QColorVector::null())
        , toXyz(QColorMatrix::null())
{
}

QColorSpacePrivate::QColorSpacePrivate(QColorSpace::ColorSpaceId colorSpaceId)
        : id(colorSpaceId)
{
    switch (colorSpaceId) {
    case QColorSpace::Undefined:
        gamut = QColorSpace::Gamut::Custom;
        transferFunction = QColorSpace::TransferFunction::Custom;
        gamma = 0.0f;
        description = QStringLiteral("Undefined");
        break;
    case QColorSpace::SRgb:
        gamut = QColorSpace::Gamut::SRgb;
        transferFunction = QColorSpace::TransferFunction::SRgb;
        gamma = 2.31f; // ?
        description = QStringLiteral("sRGB");
        break;
    case QColorSpace::SRgbLinear:
        gamut = QColorSpace::Gamut::SRgb;
        transferFunction = QColorSpace::TransferFunction::Linear;
        gamma = 1.0f;
        description = QStringLiteral("Linear sRGB");
        break;
    case QColorSpace::AdobeRgb:
        gamut = QColorSpace::Gamut::AdobeRgb;
        transferFunction = QColorSpace::TransferFunction::Gamma;
        gamma = 2.19921875f; // Not quite 2.2, see https://www.adobe.com/digitalimag/pdfs/AdobeRGB1998.pdf
        description = QStringLiteral("Adobe RGB");
        break;
    case QColorSpace::DisplayP3:
        gamut = QColorSpace::Gamut::DciP3D65;
        transferFunction = QColorSpace::TransferFunction::SRgb;
        gamma = 2.31f; // ?
        description = QStringLiteral("Display P3");
        break;
    case QColorSpace::ProPhotoRgb:
        gamut = QColorSpace::Gamut::ProPhotoRgb;
        transferFunction = QColorSpace::TransferFunction::ProPhotoRgb;
        gamma = 1.8f;
        description = QStringLiteral("ProPhoto RGB");
        break;
    case QColorSpace::Bt2020:
        gamut = QColorSpace::Gamut::Bt2020;
        transferFunction = QColorSpace::TransferFunction::Bt2020;
        gamma = 2.1f; // ?
        description = QStringLiteral("BT.2020");
        break;
    case QColorSpace::Unknown:
        qWarning("Can not create an unknown color space");
        Q_FALLTHROUGH();
    default:
        Q_UNREACHABLE();
    }
    initialize();
}

QColorSpacePrivate::QColorSpacePrivate(QColorSpace::Gamut gamut, QColorSpace::TransferFunction fun, float gamma)
        : gamut(gamut)
        , transferFunction(fun)
        , gamma(gamma)
{
    if (!identifyColorSpace())
        id = QColorSpace::Unknown;
    initialize();
}

QColorSpacePrivate::QColorSpacePrivate(const QColorSpacePrimaries &primaries,
                                       QColorSpace::TransferFunction fun,
                                       float gamma)
        : gamut(QColorSpace::Gamut::Custom)
        , transferFunction(fun)
        , gamma(gamma)
{
    Q_ASSERT(primaries.areValid());
    toXyz = primaries.toXyzMatrix();
    whitePoint = QColorVector(primaries.whitePoint);
    if (!identifyColorSpace())
        id = QColorSpace::Unknown;
    setTransferFunction();
}

bool QColorSpacePrivate::identifyColorSpace()
{
    switch (gamut) {
    case QColorSpace::Gamut::SRgb:
        if (transferFunction == QColorSpace::TransferFunction::SRgb) {
            id = QColorSpace::SRgb;
            description = QStringLiteral("sRGB");
            return true;
        }
        if (transferFunction == QColorSpace::TransferFunction::Linear) {
            id = QColorSpace::SRgbLinear;
            description = QStringLiteral("Linear sRGB");
            return true;
        }
        break;
    case QColorSpace::Gamut::AdobeRgb:
        if (transferFunction == QColorSpace::TransferFunction::Gamma) {
            if (qAbs(gamma - 2.19921875f) < (1/1024.0f)) {
                id = QColorSpace::AdobeRgb;
                description = QStringLiteral("Adobe RGB");
                return true;
            }
        }
        break;
    case QColorSpace::Gamut::DciP3D65:
        if (transferFunction == QColorSpace::TransferFunction::SRgb) {
            id = QColorSpace::DisplayP3;
            description = QStringLiteral("Display P3");
            return true;
        }
        break;
    case QColorSpace::Gamut::ProPhotoRgb:
        if (transferFunction == QColorSpace::TransferFunction::ProPhotoRgb) {
            id = QColorSpace::ProPhotoRgb;
            description = QStringLiteral("ProPhoto RGB");
            return true;
        }
        if (transferFunction == QColorSpace::TransferFunction::Gamma) {
            // ProPhoto RGB's curve is effectively gamma 1.8 for 8bit precision.
            if (qAbs(gamma - 1.8f) < (1/1024.0f)) {
                id = QColorSpace::ProPhotoRgb;
                description = QStringLiteral("ProPhoto RGB");
                return true;
            }
        }
        break;
    case QColorSpace::Gamut::Bt2020:
        if (transferFunction == QColorSpace::TransferFunction::Bt2020) {
            id = QColorSpace::Bt2020;
            description = QStringLiteral("BT.2020");
            return true;
        }
        break;
    default:
        break;
    }
    return false;
}

void QColorSpacePrivate::initialize()
{
    setToXyzMatrix();
    setTransferFunction();
}

void QColorSpacePrivate::setToXyzMatrix()
{
    if (gamut == QColorSpace::Gamut::Custom) {
        toXyz = QColorMatrix::null();
        whitePoint = QColorVector::D50();
        return;
    }
    QColorSpacePrimaries primaries(gamut);
    toXyz = primaries.toXyzMatrix();
    whitePoint = QColorVector(primaries.whitePoint);
}

void QColorSpacePrivate::setTransferFunction()
{
    switch (transferFunction) {
    case QColorSpace::TransferFunction::Linear:
        trc[0].m_type = QColorTrc::Type::Function;
        trc[0].m_fun = QColorTransferFunction();
        break;
    case QColorSpace::TransferFunction::Gamma:
        trc[0].m_type = QColorTrc::Type::Function;
        trc[0].m_fun = QColorTransferFunction::fromGamma(gamma);
        break;
    case QColorSpace::TransferFunction::SRgb:
        trc[0].m_type = QColorTrc::Type::Function;
        trc[0].m_fun = QColorTransferFunction::fromSRgb();
        break;
    case QColorSpace::TransferFunction::ProPhotoRgb:
        trc[0].m_type = QColorTrc::Type::Function;
        trc[0].m_fun = QColorTransferFunction::fromProPhotoRgb();
        break;
    case QColorSpace::TransferFunction::Bt2020:
        trc[0].m_type = QColorTrc::Type::Function;
        trc[0].m_fun = QColorTransferFunction::fromBt2020();
        break;
    case QColorSpace::TransferFunction::Custom:
        break;
    default:
        Q_UNREACHABLE();
        break;
    }
    trc[1] = trc[0];
    trc[2] = trc[0];
}

QColorTransform QColorSpacePrivate::transformationToColorSpace(const QColorSpacePrivate *out) const
{
    Q_ASSERT(out);
    QColorTransform combined;
    combined.d_ptr.reset(new QColorTransformPrivate);
    combined.d_ptr->colorSpaceIn = this;
    combined.d_ptr->colorSpaceOut = out;
    combined.d_ptr->colorMatrix = out->toXyz.inverted() * toXyz;
    return combined;
}

/*!
    \class QColorSpace
    \brief The QColorSpace class provides a color space abstraction.
    \since 5.14

    \ingroup painting
    \ingroup appearance
    \inmodule QtGui

    Color values can be interpreted in different ways, and based on the interpretation
    can live in different spaces. We call this \e {color spaces}.

    QColorSpace provides access to creating several predefined color spaces and
    can generate QColorTransforms for converting colors from one color space to
    another.

    QColorSpace can also represent color spaces defined by ICC profiles or embedded
    in images, that do not otherwise fit the predefined color spaces.

    A color space can generally speaking be conceived as a combination of a transfer
    function and a gamut. The gamut defines which colors the color space can represent.
    A color space that can represent a wider range of colors is also known as a
    wide-gamut color space. The gamut is defined by three primary colors that represent
    exactly how red, green, and blue look in this particular color space, and a white
    color that represents where and how bright pure white is.

    The transfer function or gamma curve determines how each component in the
    color space is encoded. These are used because human perception does not operate
    linearly, and the transfer functions try to ensure that colors will seem evenly
    spaced to human eyes.
*/


/*!
    \enum QColorSpace::ColorSpaceId

    Predefined color spaces.

    \value Undefined An empty, invalid or unsupported color space.
    \value Unknown A valid color space that doesn't match any of the predefined color spaces.
    \value SRgb The sRGB color space, which Qt operates in by default. It is a close approximation
    of how most classic monitors operate, and a mode most software and hardware support.
    \l{http://www.color.org/chardata/rgb/srgb.xalter}{ICC registration of sRGB}.
    \value SRgbLinear The sRGB color space with linear gamma. Useful for gamma-corrected blending.
    \value AdobeRgb The Adobe RGB color space is a classic wide-gamut color space, using a gamma of 2.2.
    \l{http://www.color.org/chardata/rgb/adobergb.xalter}{ICC registration of Adobe RGB (1998)}
    \value DisplayP3 A color-space using the primaries of DCI-P3, but with the whitepoint and transfer
    function of sRGB. Common in modern wide-gamut screens.
    \l{http://www.color.org/chardata/rgb/DCIP3.xalter}{ICC registration of DCI-P3}
    \value ProPhotoRgb The Pro Photo RGB color space, also known as ROMM RGB is a very wide gamut color space.
    \l{http://www.color.org/chardata/rgb/rommrgb.xalter}{ICC registration of ROMM RGB}
    \value Bt2020 BT.2020 also known as Rec.2020 is the color space of HDR TVs.
    \l{http://www.color.org/chardata/rgb/BT2020.xalter}{ICC registration of BT.2020}
*/

/*!
    \enum QColorSpace::Gamut

    Predefined gamuts, or sets of primary colors.

    \value Custom The gamut is undefined or does not match any predefined sets.
    \value SRgb The sRGB gamut
    \value AdobeRgb The Adobe RGB gamut
    \value DciP3D65 The DCI-P3 gamut with the D65 whitepoint
    \value ProPhotoRgb The ProPhoto RGB gamut with the D50 whitepoint
    \value Bt2020 The BT.2020 gamut
*/

/*!
    \enum QColorSpace::TransferFunction

    Predefined transfer functions or gamma curves.

    \value Custom The custom or null transfer function
    \value Linear The linear transfer functions
    \value Gamma A transfer function that is a real gamma curve based on the value of gamma()
    \value SRgb The sRGB transfer function, composed of linear and gamma parts
    \value ProPhotoRgb The ProPhoto RGB transfer function, composed of linear and gamma parts
    \value Bt2020 The BT.2020 transfer function, composed of linear and gamma parts
*/

/*!
    Creates a new colorspace object that represents \a colorSpaceId.
 */
QColorSpace::QColorSpace(QColorSpace::ColorSpaceId colorSpaceId)
{
    static QExplicitlySharedDataPointer<QColorSpacePrivate> predefinedColorspacePrivates[QColorSpace::Bt2020];
    if (colorSpaceId <= QColorSpace::Unknown) {
        if (!predefinedColorspacePrivates[0])
            predefinedColorspacePrivates[0] = new QColorSpacePrivate(QColorSpace::Undefined);
        d_ptr = predefinedColorspacePrivates[0]; // unknown and undefined both returns the static undefined colorspace.
    } else {
        if (!predefinedColorspacePrivates[colorSpaceId - 1])
            predefinedColorspacePrivates[colorSpaceId - 1] = new QColorSpacePrivate(colorSpaceId);
        d_ptr = predefinedColorspacePrivates[colorSpaceId - 1];
    }

    Q_ASSERT(colorSpaceId == QColorSpace::Undefined || isValid());
}

/*!
    Creates a custom color space with the gamut \a gamut, using the transfer function \a fun and
    optionally \a gamma.
 */
QColorSpace::QColorSpace(QColorSpace::Gamut gamut, QColorSpace::TransferFunction fun, float gamma)
        : d_ptr(new QColorSpacePrivate(gamut, fun, gamma))
{
}

/*!
    Creates a custom color space with the gamut \a gamut, using a gamma transfer function of
    \a gamma.
 */
QColorSpace::QColorSpace(QColorSpace::Gamut gamut, float gamma)
        : d_ptr(new QColorSpacePrivate(gamut, TransferFunction::Gamma, gamma))
{
}

/*!
    Creates a custom colorspace with a gamut based on the chromaticities of the primary colors \a whitePoint,
    \a redPoint, \a greenPoint and \a bluePoint, and using the transfer function \a fun and optionally \a gamma.
 */
QColorSpace::QColorSpace(const QPointF &whitePoint, const QPointF &redPoint,
                         const QPointF &greenPoint, const QPointF &bluePoint,
                         QColorSpace::TransferFunction fun, float gamma)
{
    QColorSpacePrimaries primaries(whitePoint, redPoint, greenPoint, bluePoint);
    if (!primaries.areValid()) {
        qWarning() << "QColorSpace attempted constructed from invalid primaries:" << whitePoint << redPoint << greenPoint << bluePoint;
        d_ptr = QColorSpace(QColorSpace::Undefined).d_ptr;
        return;
    }
    d_ptr = new QColorSpacePrivate(primaries, fun, gamma);
}

QColorSpace::~QColorSpace()
{
}

QColorSpace::QColorSpace(QColorSpace &&colorSpace)
        : d_ptr(std::move(colorSpace.d_ptr))
{
}

QColorSpace::QColorSpace(const QColorSpace &colorSpace)
        : d_ptr(colorSpace.d_ptr)
{
}

QColorSpace &QColorSpace::operator=(QColorSpace &&colorSpace)
{
    d_ptr = std::move(colorSpace.d_ptr);
    return *this;
}

QColorSpace &QColorSpace::operator=(const QColorSpace &colorSpace)
{
    d_ptr = colorSpace.d_ptr;
    return *this;
}

/*!
    Returns the id of the predefined color space this object
    represents or \c Unknown if it doesn't match any of them.
*/
QColorSpace::ColorSpaceId QColorSpace::colorSpaceId() const noexcept
{
    return d_ptr->id;
}

/*!
    Returns the predefined gamut of the color space
    or \c Gamut::Custom if it doesn't match any of them.
*/
QColorSpace::Gamut QColorSpace::gamut() const noexcept
{
    return d_ptr->gamut;
}

/*!
    Returns the predefined transfer function of the color space
    or \c TransferFunction::Custom if it doesn't match any of them.

    \sa gamma()
*/
QColorSpace::TransferFunction QColorSpace::transferFunction() const noexcept
{
    return d_ptr->transferFunction;
}

/*!
    Returns the gamma value of color spaces with \c TransferFunction::Gamma,
    an approximate gamma value for other predefined color spaces, or
    0.0 if no approximate gamma is known.

    \sa transferFunction()
*/
float QColorSpace::gamma() const noexcept
{
    return d_ptr->gamma;
}

/*!
    Returns an ICC profile representing the color space.

    If the color space was generated from an ICC profile, that profile
    is returned, otherwise one is generated.

    \note Even invalid color spaces may return the ICC profile if they
    were generated from one, to allow applications to implement wider
    support themselves.

    \sa fromIccProfile()
*/
QByteArray QColorSpace::iccProfile() const
{
    if (!d_ptr->iccProfile.isEmpty())
        return d_ptr->iccProfile;
    if (!isValid())
        return QByteArray();
    return QIcc::toIccProfile(*this);
}

/*!
    Creates a QColorSpace from ICC profile \a iccProfile.

    \note Not all ICC profiles are supported. QColorSpace only supports
    RGB-XYZ ICC profiles that are three-component matrix-based.

    If the ICC profile is not supported an invalid QColorSpace is returned
    where you can still read the original ICC profile using iccProfile().

    \sa iccProfile()
*/
QColorSpace QColorSpace::fromIccProfile(const QByteArray &iccProfile)
{
    QColorSpace colorSpace;
    if (QIcc::fromIccProfile(iccProfile, &colorSpace))
        return colorSpace;
    colorSpace.d_ptr->id = QColorSpace::Undefined;
    colorSpace.d_ptr->iccProfile = iccProfile;
    return colorSpace;
}

/*!
    Returns \c true if the color space is valid.
*/
bool QColorSpace::isValid() const noexcept
{
    return d_ptr->id != QColorSpace::Undefined && d_ptr->toXyz.isValid()
        && d_ptr->trc[0].isValid() && d_ptr->trc[1].isValid() && d_ptr->trc[2].isValid();
}

/*!
    \relates QColorSpace
    Returns \c true if colorspace \a colorSpace1 is equal to colorspace \a colorSpace2;
    otherwise returns \c false
*/
bool operator==(const QColorSpace &colorSpace1, const QColorSpace &colorSpace2)
{
    if (colorSpace1.d_ptr == colorSpace2.d_ptr)
        return true;

    if (colorSpace1.colorSpaceId() == QColorSpace::Undefined && colorSpace2.colorSpaceId() == QColorSpace::Undefined)
        return colorSpace1.d_ptr->iccProfile == colorSpace2.d_ptr->iccProfile;

    if (colorSpace1.colorSpaceId() != QColorSpace::Unknown && colorSpace2.colorSpaceId() != QColorSpace::Unknown)
        return colorSpace1.colorSpaceId() == colorSpace2.colorSpaceId();

    if (colorSpace1.gamut() != QColorSpace::Gamut::Custom && colorSpace2.gamut() != QColorSpace::Gamut::Custom) {
        if (colorSpace1.gamut() != colorSpace2.gamut())
            return false;
    } else {
        if (colorSpace1.d_ptr->toXyz != colorSpace2.d_ptr->toXyz)
            return false;
    }

    if (colorSpace1.transferFunction() != QColorSpace::TransferFunction::Custom &&
            colorSpace2.transferFunction() != QColorSpace::TransferFunction::Custom) {
        if (colorSpace1.transferFunction() != colorSpace2.transferFunction())
            return false;
        if (colorSpace1.transferFunction() == QColorSpace::TransferFunction::Gamma)
            return colorSpace1.gamma() == colorSpace2.gamma();
        return true;
    }

    if (colorSpace1.d_ptr->trc[0] != colorSpace2.d_ptr->trc[0] ||
        colorSpace1.d_ptr->trc[1] != colorSpace2.d_ptr->trc[1] ||
        colorSpace1.d_ptr->trc[2] != colorSpace2.d_ptr->trc[2])
        return false;

    return true;
}

/*!
    \fn bool operator!=(const QColorSpace &colorSpace1, const QColorSpace &colorSpace2)
    \relates QColorSpace

    Returns \c true if colorspace \a colorSpace1 is not equal to colorspace \a colorSpace2;
    otherwise returns \c false
*/

/*!
    Generates and returns a color space transformation from this color space to
    \a colorspace.
*/
QColorTransform QColorSpace::transformationToColorSpace(const QColorSpace &colorspace) const
{
    if (!isValid() || !colorspace.isValid())
        return QColorTransform();

    return d_ptr->transformationToColorSpace(colorspace.d_ptr.constData());
}

/*!
    \internal
*/
QColorSpacePrivate *QColorSpace::d_func()
{
    d_ptr.detach();
    return d_ptr.data();
}

/*!
    \fn const QColorSpacePrivate* QColorSpacePrivate::d_func() const
    \internal
*/

/*****************************************************************************
  QColorSpace stream functions
 *****************************************************************************/
#if !defined(QT_NO_DATASTREAM)
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QColorSpace &colorSpace)
    \relates QColorSpace

    Writes the given \a colorSpace to the given \a stream as an ICC profile.

    \sa QColorSpace::iccProfile(), {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &s, const QColorSpace &image)
{
    s << image.iccProfile();
    return s;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QColorSpace &colorSpace)
    \relates QColorSpace

    Reads a color space from the given \a stream and stores it in the given
    \a colorSpace.

    \sa QColorSpace::fromIccProfile(), {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &s, QColorSpace &colorSpace)
{
    QByteArray iccProfile;
    s >> iccProfile;
    colorSpace = QColorSpace::fromIccProfile(iccProfile);
    return s;
}
#endif // QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QColorSpace &colorSpace)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "QColorSpace(";
    dbg << colorSpace.colorSpaceId() << ", " << colorSpace.gamut() << ", " << colorSpace.transferFunction();
    dbg << ", gamma=" << colorSpace.gamma();
    dbg << ')';
    return dbg;
}
#endif

QT_END_NAMESPACE
