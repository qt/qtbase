// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcolorspace.h"
#include "qcolorspace_p.h"

#include "qcolortransform.h"
#include "qcolorclut_p.h"
#include "qcolormatrix_p.h"
#include "qcolortransferfunction_p.h"
#include "qcolortransform_p.h"
#include "qicc_p.h"

#include <qatomic.h>
#include <qmath.h>
#include <qtransform.h>

#include <qdebug.h>

QT_BEGIN_NAMESPACE

Q_CONSTINIT QBasicMutex QColorSpacePrivate::s_lutWriteLock;

Q_CONSTINIT static QAtomicPointer<QColorSpacePrivate> s_predefinedColorspacePrivates[QColorSpace::Bt2100Hlg] = {};
static void cleanupPredefinedColorspaces()
{
    for (QAtomicPointer<QColorSpacePrivate> &ptr : s_predefinedColorspacePrivates) {
        QColorSpacePrivate *prv = ptr.fetchAndStoreAcquire(nullptr);
        if (prv && !prv->ref.deref())
            delete prv;
    }
}

Q_DESTRUCTOR_FUNCTION(cleanupPredefinedColorspaces)

QColorSpace::PrimaryPoints QColorSpace::PrimaryPoints::fromPrimaries(Primaries primaries)
{
    PrimaryPoints out;
    switch (primaries) {
    case Primaries::SRgb:
        out.redPoint   = QPointF(0.640, 0.330);
        out.greenPoint = QPointF(0.300, 0.600);
        out.bluePoint  = QPointF(0.150, 0.060);
        out.whitePoint = QColorVector::D65Chromaticity();
        break;
    case Primaries::DciP3D65:
        out.redPoint   = QPointF(0.680, 0.320);
        out.greenPoint = QPointF(0.265, 0.690);
        out.bluePoint  = QPointF(0.150, 0.060);
        out.whitePoint = QColorVector::D65Chromaticity();
        break;
    case Primaries::AdobeRgb:
        out.redPoint   = QPointF(0.640, 0.330);
        out.greenPoint = QPointF(0.210, 0.710);
        out.bluePoint  = QPointF(0.150, 0.060);
        out.whitePoint = QColorVector::D65Chromaticity();
        break;
    case Primaries::ProPhotoRgb:
        out.redPoint   = QPointF(0.7347, 0.2653);
        out.greenPoint = QPointF(0.1596, 0.8404);
        out.bluePoint  = QPointF(0.0366, 0.0001);
        out.whitePoint = QColorVector::D50Chromaticity();
        break;
    case Primaries::Bt2020:
        out.redPoint   = QPointF(0.708, 0.292);
        out.greenPoint = QPointF(0.170, 0.797);
        out.bluePoint  = QPointF(0.131, 0.046);
        out.whitePoint = QColorVector::D65Chromaticity();
        break;
    default:
        Q_UNREACHABLE();
    }
    return out;
}

bool QColorSpace::PrimaryPoints::isValid() const noexcept
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

QColorMatrix qColorSpacePrimaryPointsToXyzMatrix(const QColorSpace::PrimaryPoints &primaries)
{
    // This converts to XYZ in some undefined scale.
    QColorMatrix toXyz = {
        QColorVector::fromXYChromaticity(primaries.redPoint),
        QColorVector::fromXYChromaticity(primaries.greenPoint),
        QColorVector::fromXYChromaticity(primaries.bluePoint)
    };

    // Since the white point should be (1.0, 1.0, 1.0) in the
    // input, we can figure out the scale by using the
    // inverse conversion on the white point.
    const auto wXyz = QColorVector::fromXYChromaticity(primaries.whitePoint);
    QColorVector whiteScale = toXyz.inverted().map(wXyz);

    // Now we have scaled conversion to XYZ relative to the given whitepoint
    toXyz = toXyz * QColorMatrix::fromScale(whiteScale);

    return toXyz;
}

QColorSpacePrivate::QColorSpacePrivate()
{
}

QColorSpacePrivate::QColorSpacePrivate(QColorSpace::NamedColorSpace namedColorSpace)
        : namedColorSpace(namedColorSpace)
        , colorModel(QColorSpace::ColorModel::Rgb)
{
    switch (namedColorSpace) {
    case QColorSpace::SRgb:
        primaries = QColorSpace::Primaries::SRgb;
        transferFunction = QColorSpace::TransferFunction::SRgb;
        description = QStringLiteral("sRGB");
        break;
    case QColorSpace::SRgbLinear:
        primaries = QColorSpace::Primaries::SRgb;
        transferFunction = QColorSpace::TransferFunction::Linear;
        description = QStringLiteral("Linear sRGB");
        break;
    case QColorSpace::AdobeRgb:
        primaries = QColorSpace::Primaries::AdobeRgb;
        transferFunction = QColorSpace::TransferFunction::Gamma;
        gamma = 2.19921875f; // Not quite 2.2, see https://www.adobe.com/digitalimag/pdfs/AdobeRGB1998.pdf
        description = QStringLiteral("Adobe RGB");
        break;
    case QColorSpace::DisplayP3:
        primaries = QColorSpace::Primaries::DciP3D65;
        transferFunction = QColorSpace::TransferFunction::SRgb;
        description = QStringLiteral("Display P3");
        break;
    case QColorSpace::ProPhotoRgb:
        primaries = QColorSpace::Primaries::ProPhotoRgb;
        transferFunction = QColorSpace::TransferFunction::ProPhotoRgb;
        description = QStringLiteral("ProPhoto RGB");
        break;
    case QColorSpace::Bt2020:
        primaries = QColorSpace::Primaries::Bt2020;
        transferFunction = QColorSpace::TransferFunction::Bt2020;
        description = QStringLiteral("BT.2020");
        break;
    case QColorSpace::Bt2100Pq:
        primaries = QColorSpace::Primaries::Bt2020;
        transferFunction = QColorSpace::TransferFunction::St2084;
        description = QStringLiteral("BT.2100(PQ)");
        break;
    case QColorSpace::Bt2100Hlg:
        primaries = QColorSpace::Primaries::Bt2020;
        transferFunction = QColorSpace::TransferFunction::Hlg;
        description = QStringLiteral("BT.2100(HLG)");
        break;
    default:
        Q_UNREACHABLE();
    }
    initialize();
}

QColorSpacePrivate::QColorSpacePrivate(QColorSpace::Primaries primaries, QColorSpace::TransferFunction transferFunction, float gamma)
        : primaries(primaries)
        , transferFunction(transferFunction)
        , colorModel(QColorSpace::ColorModel::Rgb)
        , gamma(gamma)
{
    identifyColorSpace();
    initialize();
}

QColorSpacePrivate::QColorSpacePrivate(const QColorSpace::PrimaryPoints &primaries,
                                       QColorSpace::TransferFunction transferFunction,
                                       float gamma)
        : primaries(QColorSpace::Primaries::Custom)
        , transferFunction(transferFunction)
        , colorModel(QColorSpace::ColorModel::Rgb)
        , gamma(gamma)
        , whitePoint(QColorVector::fromXYChromaticity(primaries.whitePoint))
{
    Q_ASSERT(primaries.isValid());
    toXyz = qColorSpacePrimaryPointsToXyzMatrix(primaries);
    chad = QColorMatrix::chromaticAdaptation(whitePoint);
    toXyz = chad * toXyz;

    identifyColorSpace();
    setTransferFunction();
}

QColorSpacePrivate::QColorSpacePrivate(QPointF whitePoint,
                                       QColorSpace::TransferFunction transferFunction,
                                       float gamma)
        : primaries(QColorSpace::Primaries::Custom)
        , transferFunction(transferFunction)
        , colorModel(QColorSpace::ColorModel::Gray)
        , gamma(gamma)
        , whitePoint(QColorVector::fromXYChromaticity(whitePoint))
{
    chad = QColorMatrix::chromaticAdaptation(this->whitePoint);
    toXyz = chad;
    setTransferFunction();
}

QColorSpacePrivate::QColorSpacePrivate(QPointF whitePoint, const QList<uint16_t> &transferFunctionTable)
      : primaries(QColorSpace::Primaries::Custom)
      , transferFunction(QColorSpace::TransferFunction::Custom)
      , colorModel(QColorSpace::ColorModel::Gray)
      , gamma(0)
      , whitePoint(QColorVector::fromXYChromaticity(whitePoint))
{
    chad = QColorMatrix::chromaticAdaptation(this->whitePoint);
    toXyz = chad;
    setTransferFunctionTable(transferFunctionTable);
    setTransferFunction();
}

QColorSpacePrivate::QColorSpacePrivate(QColorSpace::Primaries primaries, const QList<uint16_t> &transferFunctionTable)
        : primaries(primaries)
        , transferFunction(QColorSpace::TransferFunction::Custom)
        , colorModel(QColorSpace::ColorModel::Rgb)
        , gamma(0)
{
    setTransferFunctionTable(transferFunctionTable);
    identifyColorSpace();
    initialize();
}

QColorSpacePrivate::QColorSpacePrivate(const QColorSpace::PrimaryPoints &primaries, const QList<uint16_t> &transferFunctionTable)
        : primaries(QColorSpace::Primaries::Custom)
        , transferFunction(QColorSpace::TransferFunction::Custom)
        , colorModel(QColorSpace::ColorModel::Rgb)
        , gamma(0)
        , whitePoint(QColorVector::fromXYChromaticity(primaries.whitePoint))
{
    Q_ASSERT(primaries.isValid());
    toXyz = qColorSpacePrimaryPointsToXyzMatrix(primaries);
    chad = QColorMatrix::chromaticAdaptation(whitePoint);
    toXyz = chad * toXyz;
    setTransferFunctionTable(transferFunctionTable);
    identifyColorSpace();
    initialize();
}

QColorSpacePrivate::QColorSpacePrivate(const QColorSpace::PrimaryPoints &primaries,
                                       const QList<uint16_t> &redTransferFunctionTable,
                                       const QList<uint16_t> &greenTransferFunctionTable,
                                       const QList<uint16_t> &blueTransferFunctionTable)
        : primaries(QColorSpace::Primaries::Custom)
        , transferFunction(QColorSpace::TransferFunction::Custom)
        , colorModel(QColorSpace::ColorModel::Rgb)
        , gamma(0)
{
    Q_ASSERT(primaries.isValid());
    toXyz = qColorSpacePrimaryPointsToXyzMatrix(primaries);
    whitePoint = QColorVector::fromXYChromaticity(primaries.whitePoint);
    chad = QColorMatrix::chromaticAdaptation(whitePoint);
    toXyz = chad * toXyz;
    setTransferFunctionTables(redTransferFunctionTable,
                              greenTransferFunctionTable,
                              blueTransferFunctionTable);
    identifyColorSpace();
}

void QColorSpacePrivate::identifyColorSpace()
{
    switch (primaries) {
    case QColorSpace::Primaries::SRgb:
        if (transferFunction == QColorSpace::TransferFunction::SRgb) {
            namedColorSpace = QColorSpace::SRgb;
            if (description.isEmpty())
                description = QStringLiteral("sRGB");
            return;
        }
        if (transferFunction == QColorSpace::TransferFunction::Linear) {
            namedColorSpace = QColorSpace::SRgbLinear;
            if (description.isEmpty())
                description = QStringLiteral("Linear sRGB");
            return;
        }
        break;
    case QColorSpace::Primaries::AdobeRgb:
        if (transferFunction == QColorSpace::TransferFunction::Gamma) {
            if (qAbs(gamma - 2.19921875f) < (1/1024.0f)) {
                namedColorSpace = QColorSpace::AdobeRgb;
                if (description.isEmpty())
                    description = QStringLiteral("Adobe RGB");
                return;
            }
        }
        break;
    case QColorSpace::Primaries::DciP3D65:
        if (transferFunction == QColorSpace::TransferFunction::SRgb) {
            namedColorSpace = QColorSpace::DisplayP3;
            if (description.isEmpty())
                description = QStringLiteral("Display P3");
            return;
        }
        break;
    case QColorSpace::Primaries::ProPhotoRgb:
        if (transferFunction == QColorSpace::TransferFunction::ProPhotoRgb) {
            namedColorSpace = QColorSpace::ProPhotoRgb;
            if (description.isEmpty())
                description = QStringLiteral("ProPhoto RGB");
            return;
        }
        if (transferFunction == QColorSpace::TransferFunction::Gamma) {
            // ProPhoto RGB's curve is effectively gamma 1.8 for 8bit precision.
            if (qAbs(gamma - 1.8f) < (1/1024.0f)) {
                namedColorSpace = QColorSpace::ProPhotoRgb;
                if (description.isEmpty())
                    description = QStringLiteral("ProPhoto RGB");
                return;
            }
        }
        break;
    case QColorSpace::Primaries::Bt2020:
        if (transferFunction == QColorSpace::TransferFunction::Bt2020) {
            namedColorSpace = QColorSpace::Bt2020;
            if (description.isEmpty())
                description = QStringLiteral("BT.2020");
            return;
        }
        if (transferFunction == QColorSpace::TransferFunction::St2084) {
            namedColorSpace = QColorSpace::Bt2100Pq;
            if (description.isEmpty())
                description = QStringLiteral("BT.2100(PQ)");
            return;
        }
        if (transferFunction == QColorSpace::TransferFunction::Hlg) {
            namedColorSpace = QColorSpace::Bt2100Hlg;
            if (description.isEmpty())
                description = QStringLiteral("BT.2100(HLG)");
            return;
        }
        break;
    default:
        break;
    }

    namedColorSpace = Unknown;
}

void QColorSpacePrivate::initialize()
{
    setToXyzMatrix();
    setTransferFunction();
}

void QColorSpacePrivate::setToXyzMatrix()
{
    if (primaries == QColorSpace::Primaries::Custom) {
        toXyz = QColorMatrix();
        whitePoint = QColorVector::D50();
        return;
    }
    auto colorSpacePrimaries = QColorSpace::PrimaryPoints::fromPrimaries(primaries);
    toXyz = qColorSpacePrimaryPointsToXyzMatrix(colorSpacePrimaries);
    whitePoint = QColorVector::fromXYChromaticity(colorSpacePrimaries.whitePoint);
    chad = QColorMatrix::chromaticAdaptation(whitePoint);
    toXyz = chad * toXyz;
}

void QColorSpacePrivate::setTransferFunctionTable(const QList<uint16_t> &transferFunctionTable)
{
    QColorTransferTable table(transferFunctionTable.size(), transferFunctionTable);
    if (!table.isEmpty() && !table.checkValidity()) {
        qWarning() << "Invalid transfer function table given to QColorSpace";
        trc[0].m_type = QColorTrc::Type::Uninitialized;
        return;
    }
    transferFunction = QColorSpace::TransferFunction::Custom;
    QColorTransferFunction curve;
    if (table.asColorTransferFunction(&curve)) {
        // Table recognized as a specific curve
        if (curve.isIdentity()) {
            transferFunction = QColorSpace::TransferFunction::Linear;
            gamma = 1.0f;
        } else if (curve.isSRgb()) {
            transferFunction = QColorSpace::TransferFunction::SRgb;
        }
        trc[0].m_type = QColorTrc::Type::ParameterizedFunction;
        trc[0].m_fun = curve;
    } else {
        trc[0].m_type = QColorTrc::Type::Table;
        trc[0].m_table = table;
    }
}

void QColorSpacePrivate::setTransferFunctionTables(const QList<uint16_t> &redTransferFunctionTable,
                                                   const QList<uint16_t> &greenTransferFunctionTable,
                                                   const QList<uint16_t> &blueTransferFunctionTable)
{
    QColorTransferTable redTable(redTransferFunctionTable.size(), redTransferFunctionTable);
    QColorTransferTable greenTable(greenTransferFunctionTable.size(), greenTransferFunctionTable);
    QColorTransferTable blueTable(blueTransferFunctionTable.size(), blueTransferFunctionTable);
    if (!redTable.isEmpty() && !greenTable.isEmpty() && !blueTable.isEmpty() &&
        !redTable.checkValidity() && !greenTable.checkValidity() && !blueTable.checkValidity()) {
        qWarning() << "Invalid transfer function table given to QColorSpace";
        trc[0].m_type = QColorTrc::Type::Uninitialized;
        trc[1].m_type = QColorTrc::Type::Uninitialized;
        trc[2].m_type = QColorTrc::Type::Uninitialized;
        return;
    }
    transferFunction = QColorSpace::TransferFunction::Custom;
    QColorTransferFunction curve;
    if (redTable.asColorTransferFunction(&curve)) {
        trc[0].m_type = QColorTrc::Type::ParameterizedFunction;
        trc[0].m_fun = curve;
    } else {
        trc[0].m_type = QColorTrc::Type::Table;
        trc[0].m_table = redTable;
    }
    if (greenTable.asColorTransferFunction(&curve)) {
        trc[1].m_type = QColorTrc::Type::ParameterizedFunction;
        trc[1].m_fun = curve;
    } else {
        trc[1].m_type = QColorTrc::Type::Table;
        trc[1].m_table = greenTable;
    }
    if (blueTable.asColorTransferFunction(&curve)) {
        trc[2].m_type = QColorTrc::Type::ParameterizedFunction;
        trc[2].m_fun = curve;
    } else {
        trc[2].m_type = QColorTrc::Type::Table;
        trc[2].m_table = blueTable;
    }
    lut.generated.storeRelease(0);
}

void QColorSpacePrivate::setTransferFunction()
{
    switch (transferFunction) {
    case QColorSpace::TransferFunction::Linear:
        trc[0] = QColorTransferFunction();
        if (qFuzzyIsNull(gamma))
            gamma = 1.0f;
        break;
    case QColorSpace::TransferFunction::Gamma:
        trc[0] = QColorTransferFunction::fromGamma(gamma);
        break;
    case QColorSpace::TransferFunction::SRgb:
        trc[0] = QColorTransferFunction::fromSRgb();
        if (qFuzzyIsNull(gamma))
            gamma = 2.31f;
        break;
    case QColorSpace::TransferFunction::ProPhotoRgb:
        trc[0] = QColorTransferFunction::fromProPhotoRgb();
        if (qFuzzyIsNull(gamma))
            gamma = 1.8f;
        break;
    case QColorSpace::TransferFunction::Bt2020:
        trc[0] = QColorTransferFunction::fromBt2020();
        if (qFuzzyIsNull(gamma))
            gamma = 2.1f;
        break;
    case QColorSpace::TransferFunction::St2084:
        trc[0] = QColorTransferGenericFunction::pq();
        break;
    case QColorSpace::TransferFunction::Hlg:
        trc[0] = QColorTransferGenericFunction::hlg();
        break;
    case QColorSpace::TransferFunction::Custom:
        break;
    default:
        Q_UNREACHABLE();
        break;
    }
    trc[1] = trc[0];
    trc[2] = trc[0];
    lut.generated.storeRelease(0);
}

QColorTransform QColorSpacePrivate::transformationToColorSpace(const QColorSpacePrivate *out) const
{
    Q_ASSERT(out);
    QColorTransform combined;
    auto ptr = new QColorTransformPrivate;
    combined.d = ptr;
    ptr->colorSpaceIn = this;
    ptr->colorSpaceOut = out;
    if (isThreeComponentMatrix())
        ptr->colorMatrix = toXyz;
    else
        ptr->colorMatrix = QColorMatrix::identity();
    if (out->isThreeComponentMatrix())
        ptr->colorMatrix = out->toXyz.inverted() * ptr->colorMatrix;
    if (ptr->isIdentity())
        return QColorTransform();
    return combined;
}

QColorTransform QColorSpacePrivate::transformationToXYZ() const
{
    QColorTransform transform;
    auto ptr = new QColorTransformPrivate;
    transform.d = ptr;
    ptr->colorSpaceIn = this;
    ptr->colorSpaceOut = this;
    if (isThreeComponentMatrix())
        ptr->colorMatrix = toXyz;
    else
        ptr->colorMatrix = QColorMatrix::identity();
    // Convert to XYZ relative to our white point, not the regular D50 white point.
    if (!chad.isNull())
        ptr->colorMatrix = chad.inverted() * ptr->colorMatrix;
    return transform;
}

bool QColorSpacePrivate::isThreeComponentMatrix() const
{
    return transformModel == QColorSpace::TransformModel::ThreeComponentMatrix;
}

void QColorSpacePrivate::clearElementListProcessingForEdit()
{
    Q_ASSERT(transformModel == QColorSpace::TransformModel::ElementListProcessing);
    Q_ASSERT(primaries == QColorSpace::Primaries::Custom);
    Q_ASSERT(transferFunction == QColorSpace::TransferFunction::Custom);

    transformModel = QColorSpace::TransformModel::ThreeComponentMatrix;
    colorModel = QColorSpace::ColorModel::Rgb;
    isPcsLab = false;
    mAB.clear();
    mBA.clear();
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

    A color space can generally speaking be conceived as a combination of set of primary
    colors and a transfer function. The primaries defines the axes of the color space, and
    the transfer function how values are mapped on the axes.
    The primaries are for ColorModel::Rgb color spaces defined by three primary colors that
    represent exactly how red, green, and blue look in this particular color space, and a white
    color that represents where and how bright pure white is. For grayscale color spaces, only
    a single white primary is needed. The range of colors expressible by the primary colors is
    called the gamut, and a color space that can represent a wider range of colors is also
    known as a wide-gamut color space.

    The transfer function or gamma curve determines how each component in the
    color space is encoded. These are used because human perception does not operate
    linearly, and the transfer functions try to ensure that colors will seem evenly
    spaced to human eyes.
*/


/*!
    \enum QColorSpace::NamedColorSpace

    Predefined color spaces.

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
    \value [since 6.8] Bt2020 BT.2020, also known as Rec.2020 is a basic colorspace of HDR TVs.
    \l{http://www.color.org/chardata/rgb/BT2020.xalter}{ICC registration of BT.2020}
    \value [since 6.8] Bt2100Pq BT.2100(PQ), also known as Rec.2100 or HDR10 is an HDR encoding with the same
    primaries as Bt2020 but using the Perceptual Quantizer transfer function.
    \l{http://www.color.org/chardata/rgb/BT2100.xalter}{ICC registration of BT.2100}
    \value [since 6.8] Bt2100Hlg BT.2100 (HLG) is an HDR encoding with the same
    primaries as Bt2020 but using the Hybrid Log-Gamma transfer function.
*/

/*!
    \enum QColorSpace::Primaries

    Predefined sets of primary colors.

    \value Custom The primaries are undefined or does not match any predefined sets.
    \value SRgb The sRGB primaries
    \value AdobeRgb The Adobe RGB primaries
    \value DciP3D65 The DCI-P3 primaries with the D65 whitepoint
    \value ProPhotoRgb The ProPhoto RGB primaries with the D50 whitepoint
    \value [since 6.8] Bt2020 The BT.2020 primaries with a D65 whitepoint
*/

/*!
    \enum QColorSpace::TransferFunction

    Predefined transfer functions or gamma curves.

    \value Custom The custom or null transfer function
    \value Linear The linear transfer functions
    \value Gamma A transfer function that is a real gamma curve based on the value of gamma()
    \value SRgb The sRGB transfer function, composed of linear and gamma parts
    \value ProPhotoRgb The ProPhoto RGB transfer function, composed of linear and gamma parts
    \value [since 6.8] Bt2020 The BT.2020 transfer function, composited of linear and gamma parts
    \value [since 6.8] St2084 The SMPTE ST 2084 transfer function, also known Perceptual Quantizer(PQ).
    \value [since 6.8] Hlg The Hybrid log-gamma transfer function.

*/

/*!
    \enum QColorSpace::TransformModel
    \since 6.8

    Defines the processing model used for color space transforms.

    \value ThreeComponentMatrix The transform consist of a matrix calculated from primaries and set of transfer functions
    for each color channel. This is very fast and used by all predefined color spaces. Any color space on this form is
    reversible and always both valid sources and targets.
    \value ElementListProcessing The transforms are one or two lists of processing elements that can do many things,
    each list only process either to the connection color space or from it. This is very flexible, but rather
    slow, and can only be set by reading ICC profiles (See  \l fromIccProfile()). Since the two lists are
    separate a color space on this form can be a valid source, but not necessarily also a valid target. When changing
    either primaries or transfer function on a color space on this type it will reset to an empty ThreeComponentMatrix form.
*/

/*!
    \enum QColorSpace::ColorModel
    \since 6.8

    Defines the color model used by the color space data.

    \value Undefined No color model
    \value Rgb An RGB color model with red, green, and blue colors. Can apply to RGB and grayscale data.
    \value Gray A gray scale color model. Can only apply to grayscale data.
    \value Cmyk Can only represent color data defined with cyan, magenta, yellow, and black colors.
    In effect only QImage::Format_CMYK32. Note Cmyk color spaces will be TransformModel::ElementListProcessing.
*/

/*!
    \class QColorSpace::PrimaryPoints
    \brief The PrimaryPoints struct contains four primary color space points.
    \inmodule QtGui
    \since 6.9

    The four CIE XY color space points describing the gamut of an RGB color space; red, green, blue, and white.

    \sa QColorSpace::Primaries
*/

/*!
    \fn QColorSpace::PrimaryPoints QColorSpace::PrimaryPoints::fromPrimaries(QColorSpace::Primaries primaries);

    Returns the four primary points making up \a primaries.
*/

/*!
    \fn QColorSpace::PrimaryPoints::isValid() const noexcept

    Returns \c true if the primary points passes the sanity check used by methods consuming QColorSpace::PrimaryPoints.
*/

/*!
    \fn QColorSpace::QColorSpace()

    Creates a new colorspace object that represents an undefined and invalid colorspace.
 */

/*!
    Creates a new colorspace object that represents a \a namedColorSpace.
 */
QColorSpace::QColorSpace(NamedColorSpace namedColorSpace)
{
    if (namedColorSpace < QColorSpace::SRgb || namedColorSpace > QColorSpace::Bt2100Hlg) {
        qWarning() << "QColorSpace attempted constructed from invalid QColorSpace::NamedColorSpace: " << int(namedColorSpace);
        return;
    }
    // The defined namespaces start at 1:
    auto &atomicRef = s_predefinedColorspacePrivates[static_cast<int>(namedColorSpace) - 1];
    QColorSpacePrivate *cspriv = atomicRef.loadAcquire();
    if (!cspriv) {
        auto *tmp = new QColorSpacePrivate(namedColorSpace);
        tmp->ref.ref();
        if (atomicRef.testAndSetOrdered(nullptr, tmp, cspriv))
            cspriv = tmp;
        else
            delete tmp;
    }
    d_ptr = cspriv;
    Q_ASSERT(isValid());
}

/*!
    Creates a custom color space with the primaries \a primaries, using the transfer function \a transferFunction and
    optionally \a gamma.
 */
QColorSpace::QColorSpace(QColorSpace::Primaries primaries, QColorSpace::TransferFunction transferFunction, float gamma)
        : d_ptr(new QColorSpacePrivate(primaries, transferFunction, gamma))
{
}

/*!
    Creates a custom color space with the primaries \a primaries, using a gamma transfer function of
    \a gamma.
 */
QColorSpace::QColorSpace(QColorSpace::Primaries primaries, float gamma)
        : d_ptr(new QColorSpacePrivate(primaries, TransferFunction::Gamma, gamma))
{
}

/*!
    Creates a custom color space with the primaries \a gamut, using a custom transfer function
    described by \a transferFunctionTable.

    The table should contain at least 2 values, and contain an monotonically increasing list
    of values from 0 to 65535.

    \since 6.1
 */
QColorSpace::QColorSpace(QColorSpace::Primaries gamut, const QList<uint16_t> &transferFunctionTable)
    : d_ptr(new QColorSpacePrivate(gamut, transferFunctionTable))
{
}

/*!
    Creates a custom grayscale color space with the white point \a whitePoint, using the transfer function \a transferFunction and
    optionally \a gamma.

    \since 6.8
*/
QColorSpace::QColorSpace(QPointF whitePoint, TransferFunction transferFunction, float gamma)
    : d_ptr(new QColorSpacePrivate(whitePoint, transferFunction, gamma))
{
}

/*!
    Creates a custom grayscale color space with white point \a whitePoint, and using the custom transfer function described by
    \a transferFunctionTable.

    \since 6.8
*/
QColorSpace::QColorSpace(QPointF whitePoint, const QList<uint16_t> &transferFunctionTable)
    : d_ptr(new QColorSpacePrivate(whitePoint, transferFunctionTable))
{
}

/*!
    Creates a custom colorspace with primaries based on the chromaticities of the primary colors \a whitePoint,
    \a redPoint, \a greenPoint and \a bluePoint, and using the transfer function \a transferFunction and optionally \a gamma.
 */
QColorSpace::QColorSpace(const QPointF &whitePoint, const QPointF &redPoint,
                         const QPointF &greenPoint, const QPointF &bluePoint,
                         QColorSpace::TransferFunction transferFunction, float gamma)
        : QColorSpace({whitePoint, redPoint, greenPoint, bluePoint}, transferFunction, gamma)
{
}

/*!
    Creates a custom colorspace with primaries based on the chromaticities of the primary colors \a primaryPoints,
    and using the transfer function \a transferFunction and optionally \a gamma.

    \since 6.9
 */
QColorSpace::QColorSpace(const PrimaryPoints &primaryPoints, TransferFunction transferFunction, float gamma)
{
    if (!primaryPoints.isValid()) {
        qWarning() << "QColorSpace attempted constructed from invalid primaries:"
                   << primaryPoints.whitePoint << primaryPoints.redPoint << primaryPoints.greenPoint << primaryPoints.bluePoint;
        return;
    }
    d_ptr = new QColorSpacePrivate(primaryPoints, transferFunction, gamma);
}

/*!
    Creates a custom color space with primaries based on the chromaticities of the primary colors \a whitePoint,
    \a redPoint, \a greenPoint and \a bluePoint, and using the custom transfer function described by
    \a transferFunctionTable.

    \since 6.1
 */
QColorSpace::QColorSpace(const QPointF &whitePoint, const QPointF &redPoint,
                         const QPointF &greenPoint, const QPointF &bluePoint,
                         const QList<uint16_t> &transferFunctionTable)
    : d_ptr(new QColorSpacePrivate({whitePoint, redPoint, greenPoint, bluePoint}, transferFunctionTable))
{
}

/*!
    Creates a custom color space with primaries based on the chromaticities of the primary colors \a whitePoint,
    \a redPoint, \a greenPoint and \a bluePoint, and using the custom transfer functions described by
    \a redTransferFunctionTable, \a greenTransferFunctionTable, and \a blueTransferFunctionTable.

    \since 6.1
 */
QColorSpace::QColorSpace(const QPointF &whitePoint, const QPointF &redPoint,
                         const QPointF &greenPoint, const QPointF &bluePoint,
                         const QList<uint16_t> &redTransferFunctionTable,
                         const QList<uint16_t> &greenTransferFunctionTable,
                         const QList<uint16_t> &blueTransferFunctionTable)
    : d_ptr(new QColorSpacePrivate({whitePoint, redPoint, greenPoint, bluePoint},
                                   redTransferFunctionTable,
                                   greenTransferFunctionTable,
                                   blueTransferFunctionTable))
{
}

QColorSpace::~QColorSpace() = default;

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QColorSpacePrivate)

QColorSpace::QColorSpace(const QColorSpace &colorSpace) noexcept = default;

/*! \fn void QColorSpace::swap(QColorSpace &other)
    \memberswap{color space}
*/

/*!
    Returns the predefined primaries of the color space
    or \c primaries::Custom if it doesn't match any of them.
*/
QColorSpace::Primaries QColorSpace::primaries() const noexcept
{
    if (Q_UNLIKELY(!d_ptr))
        return QColorSpace::Primaries::Custom;
    return d_ptr->primaries;
}

/*!
    Returns the predefined transfer function of the color space
    or \c TransferFunction::Custom if it doesn't match any of them.

    \sa gamma(), setTransferFunction(), withTransferFunction()
*/
QColorSpace::TransferFunction QColorSpace::transferFunction() const noexcept
{
    if (Q_UNLIKELY(!d_ptr))
        return QColorSpace::TransferFunction::Custom;
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
    if (Q_UNLIKELY(!d_ptr))
        return 0.0f;
    return d_ptr->gamma;
}

/*!
    Sets the transfer function to \a transferFunction and \a gamma.

    \sa transferFunction(), gamma(), withTransferFunction()
*/
void QColorSpace::setTransferFunction(QColorSpace::TransferFunction transferFunction, float gamma)
{
    if (transferFunction == TransferFunction::Custom)
        return;
    if (!d_ptr) {
        d_ptr = new QColorSpacePrivate(Primaries::Custom, transferFunction, gamma);
        return;
    }
    if (d_ptr->transferFunction == transferFunction && d_ptr->gamma == gamma)
        return;
    detach();
    if (d_ptr->transformModel == TransformModel::ElementListProcessing)
        d_ptr->clearElementListProcessingForEdit();
    d_ptr->iccProfile = {};
    d_ptr->description = QString();
    d_ptr->transferFunction = transferFunction;
    d_ptr->gamma = gamma;
    d_ptr->identifyColorSpace();
    d_ptr->setTransferFunction();
}

/*!
    Sets the transfer function to \a transferFunctionTable.

    \since 6.1
    \sa withTransferFunction()
*/
void QColorSpace::setTransferFunction(const QList<uint16_t> &transferFunctionTable)
{
    if (!d_ptr) {
        d_ptr = new QColorSpacePrivate(Primaries::Custom, transferFunctionTable);
        d_ptr->ref.ref();
        return;
    }
    detach();
    if (d_ptr->transformModel == TransformModel::ElementListProcessing)
        d_ptr->clearElementListProcessingForEdit();
    d_ptr->iccProfile = {};
    d_ptr->description = QString();
    d_ptr->setTransferFunctionTable(transferFunctionTable);
    d_ptr->gamma = 0;
    d_ptr->identifyColorSpace();
    d_ptr->setTransferFunction();
}

/*!
    Sets the transfer functions to \a redTransferFunctionTable,
    \a greenTransferFunctionTable and \a blueTransferFunctionTable.

    \since 6.1
    \sa withTransferFunctions()
*/
void QColorSpace::setTransferFunctions(const QList<uint16_t> &redTransferFunctionTable,
                                       const QList<uint16_t> &greenTransferFunctionTable,
                                       const QList<uint16_t> &blueTransferFunctionTable)
{
    if (!d_ptr) {
        d_ptr = new QColorSpacePrivate();
        d_ptr->setTransferFunctionTables(redTransferFunctionTable,
                                         greenTransferFunctionTable,
                                         blueTransferFunctionTable);
        d_ptr->ref.ref();
        return;
    }
    detach();
    if (d_ptr->transformModel == TransformModel::ElementListProcessing)
        d_ptr->clearElementListProcessingForEdit();
    d_ptr->iccProfile = {};
    d_ptr->description = QString();
    d_ptr->setTransferFunctionTables(redTransferFunctionTable,
                                     greenTransferFunctionTable,
                                     blueTransferFunctionTable);
    d_ptr->gamma = 0;
    d_ptr->identifyColorSpace();
}

/*!
    Returns a copy of this color space, except using the transfer function
    \a transferFunction and \a gamma.

    \sa transferFunction(), gamma(), setTransferFunction()
*/
QColorSpace QColorSpace::withTransferFunction(QColorSpace::TransferFunction transferFunction, float gamma) const
{
    if (!isValid() || transferFunction == TransferFunction::Custom)
        return *this;
    if (d_ptr->transferFunction == transferFunction && d_ptr->gamma == gamma)
        return *this;
    QColorSpace out(*this);
    out.setTransferFunction(transferFunction, gamma);
    return out;
}

/*!
    Returns a copy of this color space, except using the transfer function
    described by \a transferFunctionTable.

    \since 6.1
    \sa transferFunction(), setTransferFunction()
*/
QColorSpace QColorSpace::withTransferFunction(const QList<uint16_t> &transferFunctionTable) const
{
    if (!isValid())
        return *this;
    QColorSpace out(*this);
    out.setTransferFunction(transferFunctionTable);
    return out;
}

/*!
    Returns a copy of this color space, except using the transfer functions
    described by \a redTransferFunctionTable, \a greenTransferFunctionTable and
    \a blueTransferFunctionTable.

    \since 6.1
    \sa setTransferFunctions()
*/
QColorSpace QColorSpace::withTransferFunctions(const QList<uint16_t> &redTransferFunctionTable,
                                               const QList<uint16_t> &greenTransferFunctionTable,
                                               const QList<uint16_t> &blueTransferFunctionTable) const
{
    if (!isValid())
        return *this;
    QColorSpace out(*this);
    out.setTransferFunctions(redTransferFunctionTable, greenTransferFunctionTable, blueTransferFunctionTable);
    return out;
}

/*!
    Sets the primaries to those of the \a primariesId set.

    \sa primaries()
*/
void QColorSpace::setPrimaries(QColorSpace::Primaries primariesId)
{
    if (primariesId == Primaries::Custom)
        return;
    if (!d_ptr) {
        d_ptr = new QColorSpacePrivate(primariesId, TransferFunction::Custom, 0.0f);
        return;
    }
    if (d_ptr->primaries == primariesId)
        return;
    detach();
    if (d_ptr->transformModel == TransformModel::ElementListProcessing)
        d_ptr->clearElementListProcessingForEdit();
    d_ptr->iccProfile = {};
    d_ptr->description = QString();
    d_ptr->primaries = primariesId;
    d_ptr->colorModel = QColorSpace::ColorModel::Rgb;
    d_ptr->identifyColorSpace();
    d_ptr->setToXyzMatrix();
}

/*!
    Set primaries to the chromaticities of \a whitePoint, \a redPoint, \a greenPoint
    and \a bluePoint.

    \sa primaries(), setPrimaryPoints()
*/
void QColorSpace::setPrimaries(const QPointF &whitePoint, const QPointF &redPoint,
                               const QPointF &greenPoint, const QPointF &bluePoint)
{
    setPrimaryPoints({whitePoint, redPoint, greenPoint, bluePoint});
}

/*!
    Set all primaries to the chromaticities of \a primaryPoints.

    \since 6.9
    \sa primaries(), primaryPoints()
*/
void QColorSpace::setPrimaryPoints(const QColorSpace::PrimaryPoints &primaryPoints)
{
    if (!primaryPoints.isValid())
        return;
    if (!d_ptr) {
        d_ptr = new QColorSpacePrivate(primaryPoints, TransferFunction::Custom, 0.0f);
        return;
    }
    QColorMatrix toXyz = qColorSpacePrimaryPointsToXyzMatrix(primaryPoints);
    QColorMatrix chad = QColorMatrix::chromaticAdaptation(QColorVector::fromXYChromaticity(primaryPoints.whitePoint));
    toXyz = chad * toXyz;
    if (QColorVector::fromXYChromaticity(primaryPoints.whitePoint) == d_ptr->whitePoint
        && toXyz == d_ptr->toXyz && chad == d_ptr->chad)
        return;
    detach();
    if (d_ptr->transformModel == TransformModel::ElementListProcessing)
        d_ptr->clearElementListProcessingForEdit();
    d_ptr->iccProfile = {};
    d_ptr->description = QString();
    d_ptr->primaries = QColorSpace::Primaries::Custom;
    d_ptr->colorModel = QColorSpace::ColorModel::Rgb;
    d_ptr->toXyz = toXyz;
    d_ptr->chad = chad;
    d_ptr->whitePoint = QColorVector::fromXYChromaticity(primaryPoints.whitePoint);
    d_ptr->identifyColorSpace();
}

/*!
    Returns the primary chromaticities, if not defined, returns null points.

    \since 6.9
    \sa primaries(), setPrimaryPoints()
*/
QColorSpace::PrimaryPoints QColorSpace::primaryPoints() const
{
    if (Q_UNLIKELY(!d_ptr))
        return {};
    QColorMatrix rawToXyz = d_ptr->chad.inverted() * d_ptr->toXyz;
    return PrimaryPoints{rawToXyz.r.toChromaticity(),
                         rawToXyz.g.toChromaticity(),
                         rawToXyz.b.toChromaticity(),
                         d_ptr->whitePoint.toChromaticity()};
}

/*!
    Returns the white point used for this color space. Returns a null QPointF if not defined.

    \since 6.8
*/
QPointF QColorSpace::whitePoint() const
{
    if (Q_UNLIKELY(!d_ptr))
        return QPointF();
    return d_ptr->whitePoint.toChromaticity();
}

/*!
    Sets the white point to used for this color space to \a whitePoint.

    \since 6.8
*/
void QColorSpace::setWhitePoint(QPointF whitePoint)
{
    if (Q_UNLIKELY(!d_ptr)) {
        d_ptr = new QColorSpacePrivate(whitePoint, TransferFunction::Custom, 0.0f);
        return;
    }
    if (QColorVector::fromXYChromaticity(whitePoint) == d_ptr->whitePoint)
        return;
    detach();
    if (d_ptr->transformModel == TransformModel::ElementListProcessing)
        d_ptr->clearElementListProcessingForEdit();
    d_ptr->iccProfile = {};
    d_ptr->description = QString();
    d_ptr->primaries = QColorSpace::Primaries::Custom;
    // An RGB color model stays RGB, a gray stays gray, but an undefined one can now be considered gray
    if (d_ptr->colorModel == QColorSpace::ColorModel::Undefined)
        d_ptr->colorModel = QColorSpace::ColorModel::Gray;
    QColorVector wXyz(QColorVector::fromXYChromaticity(whitePoint));
    if (d_ptr->transformModel == QColorSpace::TransformModel::ThreeComponentMatrix) {
        if (d_ptr->colorModel == QColorSpace::ColorModel::Rgb) {
            // Rescale toXyz to new whitepoint
            QColorMatrix rawToXyz = d_ptr->chad.inverted() * d_ptr->toXyz;
            QColorVector whiteScale = rawToXyz.inverted().map(wXyz);
            rawToXyz = rawToXyz * QColorMatrix::fromScale(whiteScale);
            d_ptr->chad = QColorMatrix::chromaticAdaptation(wXyz);
            d_ptr->toXyz = d_ptr->chad * rawToXyz;
        } else if (d_ptr->colorModel == QColorSpace::ColorModel::Gray) {
            d_ptr->chad = d_ptr->toXyz = QColorMatrix::chromaticAdaptation(wXyz);
        }
    }
    d_ptr->whitePoint = wXyz;
    d_ptr->identifyColorSpace();
}

/*!
    Returns the transfrom processing model used for this color space.

    \since 6.8
*/
QColorSpace::TransformModel QColorSpace::transformModel() const noexcept
{
    if (Q_UNLIKELY(!d_ptr))
        return QColorSpace::TransformModel::ThreeComponentMatrix;
    return d_ptr->transformModel;
}

/*!
    Returns the color model this color space can represent

    \since 6.8
*/
QColorSpace::ColorModel QColorSpace::colorModel() const noexcept
{
    if (Q_UNLIKELY(!d_ptr))
        return QColorSpace::ColorModel::Undefined;
    return d_ptr->colorModel;
}

/*!
    \internal
*/
void QColorSpace::detach()
{
    if (d_ptr)
        d_ptr.detach();
    else
        d_ptr = new QColorSpacePrivate;
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
    if (Q_UNLIKELY(!d_ptr))
        return QByteArray();
    if (!d_ptr->iccProfile.isEmpty())
        return d_ptr->iccProfile;
    if (!isValid())
        return QByteArray();
    return QIcc::toIccProfile(*this);
}

/*!
    Creates a QColorSpace from ICC profile \a iccProfile.

    \note Not all ICC profiles are supported. QColorSpace only supports
    RGB or Gray ICC profiles.

    If the ICC profile is not supported an invalid QColorSpace is returned
    where you can still read the original ICC profile using iccProfile().

    \sa iccProfile()
*/
QColorSpace QColorSpace::fromIccProfile(const QByteArray &iccProfile)
{
    QColorSpace colorSpace;
    if (QIcc::fromIccProfile(iccProfile, &colorSpace))
        return colorSpace;
    colorSpace.detach();
    colorSpace.d_ptr->iccProfile = iccProfile;
    return colorSpace;
}

/*!
    Returns \c true if the color space is valid. For a color space with \c TransformModel::ThreeComponentMatrix
    that means both primaries and transfer functions set, and implies isValidTarget().
    For a color space with \c TransformModel::ElementListProcessing it means it has a valid source transform, to
    check if it also a valid target color space use isValidTarget().

    \sa isValidTarget()
*/
bool QColorSpace::isValid() const noexcept
{
    if (!d_ptr)
        return false;
    return d_ptr->isValid();
}

/*!
    \since 6.8

    Returns \c true if the color space is a valid target color space.
*/
bool QColorSpace::isValidTarget() const noexcept
{
    if (!d_ptr)
        return false;
    if (!d_ptr->isThreeComponentMatrix())
        return !d_ptr->mBA.isEmpty();
    return d_ptr->isValid();
}

/*!
    \internal
*/
bool QColorSpacePrivate::isValid() const noexcept
{
    if (!isThreeComponentMatrix())
        return !mAB.isEmpty();
    if (!toXyz.isValid())
        return false;
    if (colorModel == QColorSpace::ColorModel::Gray) {
        if (!trc[0].isValid())
            return false;
    } else if (colorModel == QColorSpace::ColorModel::Rgb){
        if (!trc[0].isValid() || !trc[1].isValid() || !trc[2].isValid())
            return false;
    } else {
        return false;
    }
    return true;
}

/*!
    \fn bool QColorSpace::operator==(const QColorSpace &colorSpace1, const QColorSpace &colorSpace2)

    Returns \c true if colorspace \a colorSpace1 is equal to colorspace \a colorSpace2;
    otherwise returns \c false
*/

/*!
    \fn bool QColorSpace::operator!=(const QColorSpace &colorSpace1, const QColorSpace &colorSpace2)

    Returns \c true if colorspace \a colorSpace1 is not equal to colorspace \a colorSpace2;
    otherwise returns \c false
*/

static bool compareElement(const QColorSpacePrivate::TransferElement &element,
                           const QColorSpacePrivate::TransferElement &other)
{
    return element.trc[0] == other.trc[0]
        && element.trc[1] == other.trc[1]
        && element.trc[2] == other.trc[2]
        && element.trc[3] == other.trc[3];
}

static bool compareElement(const QColorMatrix &element,
                           const QColorMatrix &other)
{
    return element == other;
}

static bool compareElement(const QColorVector &element,
                           const QColorVector &other)
{
    return element == other;
}

static bool compareElement(const QColorCLUT &element,
                           const QColorCLUT &other)
{
    if (element.gridPointsX != other.gridPointsX)
        return false;
    if (element.gridPointsY != other.gridPointsY)
        return false;
    if (element.gridPointsZ != other.gridPointsZ)
        return false;
    if (element.gridPointsW != other.gridPointsW)
        return false;
    if (element.table.size() != other.table.size())
        return false;
    for (qsizetype i = 0; i < element.table.size(); ++i) {
        if (element.table[i] != other.table[i])
            return false;
    }
    return true;
}

template<typename T>
static bool compareElements(const T &element, const QColorSpacePrivate::Element &other)
{
    return compareElement(element, std::get<T>(other));
}

/*!
    \internal
*/
bool QColorSpace::equals(const QColorSpace &other) const
{
    if (d_ptr == other.d_ptr)
        return true;
    if (!d_ptr)
        return false;
    return d_ptr->equals(other.d_ptr.constData());
}

/*!
    \internal
*/
bool QColorSpacePrivate::equals(const QColorSpacePrivate *other) const
{
    if (!other)
        return false;

    if (namedColorSpace && other->namedColorSpace)
        return namedColorSpace == other->namedColorSpace;

    const bool valid1 = isValid();
    const bool valid2 = other->isValid();
    if (valid1 != valid2)
        return false;
    if (!valid1 && !valid2) {
        if (!iccProfile.isEmpty() || !other->iccProfile.isEmpty())
            return iccProfile == other->iccProfile;
        return false;
    }

    // At this point one or both color spaces are unknown, and must be compared in detail instead

    if (transformModel != other->transformModel)
        return false;

    if (!isThreeComponentMatrix()) {
        if (isPcsLab != other->isPcsLab)
            return false;
        if (colorModel != other->colorModel)
            return false;
        if (mAB.count() != other->mAB.count())
            return false;
        if (mBA.count() != other->mBA.count())
            return false;

        // Compare element types
        for (qsizetype i = 0; i < mAB.count(); ++i) {
            if (mAB[i].index() != other->mAB[i].index())
                return false;
        }
        for (qsizetype i = 0; i < mBA.count(); ++i) {
            if (mBA[i].index() != other->mBA[i].index())
                return false;
        }

        // Compare element contents
        for (qsizetype i = 0; i < mAB.count(); ++i) {
            if (!std::visit([&](auto &&elm) { return compareElements(elm, other->mAB[i]); }, mAB[i]))
                return false;
        }
        for (qsizetype i = 0; i < mBA.count(); ++i) {
            if (!std::visit([&](auto &&elm) { return compareElements(elm, other->mBA[i]); }, mBA[i]))
                return false;
        }

        return true;
    }

    if (primaries != QColorSpace::Primaries::Custom && other->primaries != QColorSpace::Primaries::Custom) {
        if (primaries != other->primaries)
            return false;
    } else {
        if (toXyz != other->toXyz)
            return false;
    }

    if (transferFunction != QColorSpace::TransferFunction::Custom && other->transferFunction != QColorSpace::TransferFunction::Custom) {
        if (transferFunction != other->transferFunction)
            return false;
        if (transferFunction == QColorSpace::TransferFunction::Gamma)
            return (qAbs(gamma - other->gamma) <= (1.0f / 512.0f));
        return true;
    }

    if (trc[0] != other->trc[0] ||
        trc[1] != other->trc[1] ||
        trc[2] != other->trc[2])
        return false;

    return true;
}

/*!
    Generates and returns a color space transformation from this color space to
    \a colorspace.
*/
QColorTransform QColorSpace::transformationToColorSpace(const QColorSpace &colorspace) const
{
    if (!isValid())
        return QColorTransform();

    if (*this == colorspace)
        return QColorTransform();
    if (!colorspace.isValidTarget()) {
        qWarning() << "QColorSpace::transformationToColorSpace: colorspace not a valid target";
        return QColorTransform();
    }

    return d_ptr->transformationToColorSpace(colorspace.d_ptr.get());
}

/*!
    Returns the color space as a QVariant.
    \since 5.15
*/
QColorSpace::operator QVariant() const
{
    return QVariant::fromValue(*this);
}

/*!
    Returns the name or short description. If a description hasn't been given
    in setDescription(), the original name of the profile is returned if the
    profile is unmodified, a guessed name is returned if the profile has been
    recognized as a known color space, otherwise an empty string is returned.

    \since 6.2
*/
QString QColorSpace::description() const noexcept
{
    if (d_ptr)
        return d_ptr->userDescription.isEmpty() ? d_ptr->description : d_ptr->userDescription;
    return QString();
}

/*!
    Sets the name or short description of the color space to \a description.

    If set to empty description() will return original or guessed descriptions
    instead.

    \since 6.2
*/
void QColorSpace::setDescription(const QString &description)
{
    detach();
    d_ptr->iccProfile = {};
    d_ptr->userDescription = description;
}

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
QDebug operator<<(QDebug dbg, const QColorSpacePrivate::TransferElement &)
{
    return dbg << ":Transfer";
}
QDebug operator<<(QDebug dbg, const QColorMatrix &)
{
    return dbg << ":Matrix";
}
QDebug operator<<(QDebug dbg, const QColorVector &)
{
    return dbg << ":Offset";
}
QDebug operator<<(QDebug dbg, const QColorCLUT &)
{
    return dbg << ":CLUT";
}
QDebug operator<<(QDebug dbg, const QList<QColorSpacePrivate::Element> &elements)
{
    for (auto &&element : elements)
        std::visit([&](auto &&elm) { dbg << elm; }, element);
    return dbg;
}
QDebug operator<<(QDebug dbg, const QColorSpace &colorSpace)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "QColorSpace(";
    if (colorSpace.d_ptr) {
        if (colorSpace.d_ptr->namedColorSpace)
            dbg << colorSpace.d_ptr->namedColorSpace << ", ";
        else
            dbg << colorSpace.colorModel() << ", ";
        if (!colorSpace.isValid()) {
            dbg << "Invalid";
            if (!colorSpace.d_ptr->iccProfile.isEmpty())
                dbg << " with profile data";
        } else if (colorSpace.d_ptr->isThreeComponentMatrix()) {
            dbg << colorSpace.primaries() << ", " << colorSpace.transferFunction();
            if (colorSpace.transferFunction() == QColorSpace::TransferFunction::Gamma)
                dbg  << "=" << colorSpace.gamma();
        } else {
            if (colorSpace.d_ptr->isPcsLab)
                dbg << "PCSLab, ";
            else
                dbg << "PCSXYZ, ";
            dbg << "A2B" << colorSpace.d_ptr->mAB;
            if (!colorSpace.d_ptr->mBA.isEmpty())
                dbg << ", B2A" << colorSpace.d_ptr->mBA;
        }
    }
    dbg << ')';
    return dbg;
}
#endif

QT_END_NAMESPACE

#include "moc_qcolorspace.cpp"
