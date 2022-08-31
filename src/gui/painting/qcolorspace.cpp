// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcolorspace.h"
#include "qcolorspace_p.h"

#include "qcolortransform.h"
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

Q_CONSTINIT static QAtomicPointer<QColorSpacePrivate> s_predefinedColorspacePrivates[QColorSpace::ProPhotoRgb] = {};
static void cleanupPredefinedColorspaces()
{
    for (QAtomicPointer<QColorSpacePrivate> &ptr : s_predefinedColorspacePrivates) {
        QColorSpacePrivate *prv = ptr.fetchAndStoreAcquire(nullptr);
        if (prv && !prv->ref.deref())
            delete prv;
    }
}

Q_DESTRUCTOR_FUNCTION(cleanupPredefinedColorspaces)

QColorSpacePrimaries::QColorSpacePrimaries(QColorSpace::Primaries primaries)
{
    switch (primaries) {
    case QColorSpace::Primaries::SRgb:
        redPoint   = QPointF(0.640, 0.330);
        greenPoint = QPointF(0.300, 0.600);
        bluePoint  = QPointF(0.150, 0.060);
        whitePoint = QColorVector::D65Chromaticity();
        break;
    case QColorSpace::Primaries::DciP3D65:
        redPoint   = QPointF(0.680, 0.320);
        greenPoint = QPointF(0.265, 0.690);
        bluePoint  = QPointF(0.150, 0.060);
        whitePoint = QColorVector::D65Chromaticity();
        break;
    case QColorSpace::Primaries::AdobeRgb:
        redPoint   = QPointF(0.640, 0.330);
        greenPoint = QPointF(0.210, 0.710);
        bluePoint  = QPointF(0.150, 0.060);
        whitePoint = QColorVector::D65Chromaticity();
        break;
    case QColorSpace::Primaries::ProPhotoRgb:
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

        if (srcCone.x && srcCone.y && srcCone.z) {
            QColorMatrix wToD50 = { { dstCone.x / srcCone.x, 0, 0 },
                                    { 0, dstCone.y / srcCone.y, 0 },
                                    { 0, 0, dstCone.z / srcCone.z } };


            QColorMatrix chromaticAdaptation = abradinv * (wToD50 * abrad);
            toXyz = chromaticAdaptation * toXyz;
        } else {
            toXyz.r = {0, 0, 0}; // set to invalid value
        }
    }

    return toXyz;
}

QColorSpacePrivate::QColorSpacePrivate()
{
}

QColorSpacePrivate::QColorSpacePrivate(QColorSpace::NamedColorSpace namedColorSpace)
        : namedColorSpace(namedColorSpace)
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
    default:
        Q_UNREACHABLE();
    }
    initialize();
}

QColorSpacePrivate::QColorSpacePrivate(QColorSpace::Primaries primaries, QColorSpace::TransferFunction transferFunction, float gamma)
        : primaries(primaries)
        , transferFunction(transferFunction)
        , gamma(gamma)
{
    identifyColorSpace();
    initialize();
}

QColorSpacePrivate::QColorSpacePrivate(const QColorSpacePrimaries &primaries,
                                       QColorSpace::TransferFunction transferFunction,
                                       float gamma)
        : primaries(QColorSpace::Primaries::Custom)
        , transferFunction(transferFunction)
        , gamma(gamma)
{
    Q_ASSERT(primaries.areValid());
    toXyz = primaries.toXyzMatrix();
    whitePoint = QColorVector(primaries.whitePoint);
    identifyColorSpace();
    setTransferFunction();
}

QColorSpacePrivate::QColorSpacePrivate(QColorSpace::Primaries primaries, const QList<uint16_t> &transferFunctionTable)
        : primaries(primaries)
        , transferFunction(QColorSpace::TransferFunction::Custom)
        , gamma(0)
{
    setTransferFunctionTable(transferFunctionTable);
    identifyColorSpace();
    initialize();
}

QColorSpacePrivate::QColorSpacePrivate(const QColorSpacePrimaries &primaries, const QList<uint16_t> &transferFunctionTable)
        : primaries(QColorSpace::Primaries::Custom)
        , transferFunction(QColorSpace::TransferFunction::Custom)
        , gamma(0)
{
    Q_ASSERT(primaries.areValid());
    toXyz = primaries.toXyzMatrix();
    whitePoint = QColorVector(primaries.whitePoint);
    setTransferFunctionTable(transferFunctionTable);
    identifyColorSpace();
    initialize();
}

QColorSpacePrivate::QColorSpacePrivate(const QColorSpacePrimaries &primaries,
                                       const QList<uint16_t> &redTransferFunctionTable,
                                       const QList<uint16_t> &greenTransferFunctionTable,
                                       const QList<uint16_t> &blueTransferFunctionTable)
        : primaries(QColorSpace::Primaries::Custom)
        , transferFunction(QColorSpace::TransferFunction::Custom)
        , gamma(0)
{
    Q_ASSERT(primaries.areValid());
    toXyz = primaries.toXyzMatrix();
    whitePoint = QColorVector(primaries.whitePoint);
    setTransferFunctionTables(redTransferFunctionTable,
                              greenTransferFunctionTable,
                              blueTransferFunctionTable);
    identifyColorSpace();
    setToXyzMatrix();
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
    QColorSpacePrimaries colorSpacePrimaries(primaries);
    toXyz = colorSpacePrimaries.toXyzMatrix();
    whitePoint = QColorVector(colorSpacePrimaries.whitePoint);
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
        if (curve.isLinear()) {
            transferFunction = QColorSpace::TransferFunction::Linear;
            gamma = 1.0f;
        } else if (curve.isSRgb()) {
            transferFunction = QColorSpace::TransferFunction::SRgb;
        }
        trc[0].m_type = QColorTrc::Type::Function;
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
        trc[0].m_type = QColorTrc::Type::Function;
        trc[0].m_fun = curve;
    } else {
        trc[0].m_type = QColorTrc::Type::Table;
        trc[0].m_table = redTable;
    }
    if (greenTable.asColorTransferFunction(&curve)) {
        trc[1].m_type = QColorTrc::Type::Function;
        trc[1].m_fun = curve;
    } else {
        trc[1].m_type = QColorTrc::Type::Table;
        trc[1].m_table = greenTable;
    }
    if (blueTable.asColorTransferFunction(&curve)) {
        trc[2].m_type = QColorTrc::Type::Function;
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
        trc[0].m_type = QColorTrc::Type::Function;
        trc[0].m_fun = QColorTransferFunction();
        if (qFuzzyIsNull(gamma))
            gamma = 1.0f;
        break;
    case QColorSpace::TransferFunction::Gamma:
        trc[0].m_type = QColorTrc::Type::Function;
        trc[0].m_fun = QColorTransferFunction::fromGamma(gamma);
        break;
    case QColorSpace::TransferFunction::SRgb:
        trc[0].m_type = QColorTrc::Type::Function;
        trc[0].m_fun = QColorTransferFunction::fromSRgb();
        if (qFuzzyIsNull(gamma))
            gamma = 2.31f;
        break;
    case QColorSpace::TransferFunction::ProPhotoRgb:
        trc[0].m_type = QColorTrc::Type::Function;
        trc[0].m_fun = QColorTransferFunction::fromProPhotoRgb();
        if (qFuzzyIsNull(gamma))
            gamma = 1.8f;
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
    ptr->colorMatrix = out->toXyz.inverted() * toXyz;
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
    ptr->colorMatrix = toXyz;
    return transform;
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
    The primaries are defined by three primary colors that represent exactly how red, green,
    and blue look in this particular color space, and a white color that represents where
    and how bright pure white is. The range of colors expressible by the primary colors is
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
*/

/*!
    \enum QColorSpace::Primaries

    Predefined sets of primary colors.

    \value Custom The primaries are undefined or does not match any predefined sets.
    \value SRgb The sRGB primaries
    \value AdobeRgb The Adobe RGB primaries
    \value DciP3D65 The DCI-P3 primaries with the D65 whitepoint
    \value ProPhotoRgb The ProPhoto RGB primaries with the D50 whitepoint
*/

/*!
    \enum QColorSpace::TransferFunction

    Predefined transfer functions or gamma curves.

    \value Custom The custom or null transfer function
    \value Linear The linear transfer functions
    \value Gamma A transfer function that is a real gamma curve based on the value of gamma()
    \value SRgb The sRGB transfer function, composed of linear and gamma parts
    \value ProPhotoRgb The ProPhoto RGB transfer function, composed of linear and gamma parts
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
    if (namedColorSpace < QColorSpace::SRgb || namedColorSpace > QColorSpace::ProPhotoRgb) {
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
    Creates a custom colorspace with a primaries based on the chromaticities of the primary colors \a whitePoint,
    \a redPoint, \a greenPoint and \a bluePoint, and using the transfer function \a transferFunction and optionally \a gamma.
 */
QColorSpace::QColorSpace(const QPointF &whitePoint, const QPointF &redPoint,
                         const QPointF &greenPoint, const QPointF &bluePoint,
                         QColorSpace::TransferFunction transferFunction, float gamma)
{
    QColorSpacePrimaries primaries(whitePoint, redPoint, greenPoint, bluePoint);
    if (!primaries.areValid()) {
        qWarning() << "QColorSpace attempted constructed from invalid primaries:" << whitePoint << redPoint << greenPoint << bluePoint;
        return;
    }
    d_ptr = new QColorSpacePrivate(primaries, transferFunction, gamma);
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

    Swaps color space \a other with this color space. This operation is very fast and
    never fails.
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
    d_ptr->description.clear();
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
    d_ptr->description.clear();
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
    d_ptr->description.clear();
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
    if (!isValid() || transferFunction == QColorSpace::TransferFunction::Custom)
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
    d_ptr->description.clear();
    d_ptr->primaries = primariesId;
    d_ptr->identifyColorSpace();
    d_ptr->setToXyzMatrix();
}

/*!
    Set primaries to the chromaticities of \a whitePoint, \a redPoint, \a greenPoint
    and \a bluePoint.

    \sa primaries()
*/
void QColorSpace::setPrimaries(const QPointF &whitePoint, const QPointF &redPoint,
                               const QPointF &greenPoint, const QPointF &bluePoint)
{
    QColorSpacePrimaries primaries(whitePoint, redPoint, greenPoint, bluePoint);
    if (!primaries.areValid())
        return;
    if (!d_ptr) {
        d_ptr = new QColorSpacePrivate(primaries, TransferFunction::Custom, 0.0f);
        return;
    }
    QColorMatrix toXyz = primaries.toXyzMatrix();
    if (QColorVector(primaries.whitePoint) == d_ptr->whitePoint && toXyz == d_ptr->toXyz)
        return;
    detach();
    d_ptr->description.clear();
    d_ptr->primaries = QColorSpace::Primaries::Custom;
    d_ptr->toXyz = toXyz;
    d_ptr->whitePoint = QColorVector(primaries.whitePoint);
    d_ptr->identifyColorSpace();
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
    colorSpace.detach();
    colorSpace.d_ptr->iccProfile = iccProfile;
    return colorSpace;
}

/*!
    Returns \c true if the color space is valid.
*/
bool QColorSpace::isValid() const noexcept
{
    return d_ptr
        && d_ptr->toXyz.isValid()
        && d_ptr->trc[0].isValid() && d_ptr->trc[1].isValid() && d_ptr->trc[2].isValid();
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

/*!
    \internal
*/
bool QColorSpace::equals(const QColorSpace &other) const
{
    if (d_ptr == other.d_ptr)
        return true;
    if (!d_ptr || !other.d_ptr)
        return false;

    if (d_ptr->namedColorSpace && other.d_ptr->namedColorSpace)
        return d_ptr->namedColorSpace == other.d_ptr->namedColorSpace;

    const bool valid1 = isValid();
    const bool valid2 = other.isValid();
    if (valid1 != valid2)
        return false;
    if (!valid1 && !valid2) {
        if (!d_ptr->iccProfile.isEmpty() || !other.d_ptr->iccProfile.isEmpty())
            return d_ptr->iccProfile == other.d_ptr->iccProfile;
    }

    // At this point one or both color spaces are unknown, and must be compared in detail instead

    if (primaries() != QColorSpace::Primaries::Custom && other.primaries() != QColorSpace::Primaries::Custom) {
        if (primaries() != other.primaries())
            return false;
    } else {
        if (d_ptr->toXyz != other.d_ptr->toXyz)
            return false;
    }

    if (transferFunction() != QColorSpace::TransferFunction::Custom &&
            other.transferFunction() != QColorSpace::TransferFunction::Custom) {
        if (transferFunction() != other.transferFunction())
            return false;
        if (transferFunction() == QColorSpace::TransferFunction::Gamma)
            return (qAbs(gamma() - other.gamma()) <= (1.0f / 512.0f));
        return true;
    }

    if (d_ptr->trc[0] != other.d_ptr->trc[0] ||
        d_ptr->trc[1] != other.d_ptr->trc[1] ||
        d_ptr->trc[2] != other.d_ptr->trc[2])
        return false;

    return true;
}

/*!
    Generates and returns a color space transformation from this color space to
    \a colorspace.
*/
QColorTransform QColorSpace::transformationToColorSpace(const QColorSpace &colorspace) const
{
    if (!isValid() || !colorspace.isValid())
        return QColorTransform();

    if (*this == colorspace)
        return QColorTransform();

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
QDebug operator<<(QDebug dbg, const QColorSpace &colorSpace)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "QColorSpace(";
    if (colorSpace.d_ptr) {
        if (colorSpace.d_ptr->namedColorSpace)
            dbg << colorSpace.d_ptr->namedColorSpace << ", ";
        dbg << colorSpace.primaries() << ", " << colorSpace.transferFunction();
        dbg << ", gamma=" << colorSpace.gamma();
    }
    dbg << ')';
    return dbg;
}
#endif

QT_END_NAMESPACE

#include "moc_qcolorspace.cpp"
