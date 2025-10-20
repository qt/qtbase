// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylandcolormanagement_p.h"
#include "qwaylanddisplay_p.h"

#include <QDebug>

#include <unistd.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

ColorManager::ColorManager(struct ::wl_registry *registry, uint32_t id, int version)
    : QtWayland::wp_color_manager_v1(registry, id, version)
{
}

ColorManager::~ColorManager()
{
    destroy();
}

void ColorManager::wp_color_manager_v1_supported_feature(uint32_t feature)
{
    switch (feature) {
    case feature_icc_v2_v4:
        mFeatures |= Feature::ICC;
        break;
    case feature_parametric:
        mFeatures |= Feature::Parametric;
        break;
    case feature_set_primaries:
        mFeatures |= Feature::SetPrimaries;
        break;
    case feature_set_tf_power:
        mFeatures |= Feature::PowerTransferFunction;
        break;
    case feature_set_luminances:
        mFeatures |= Feature::SetLuminances;
        break;
    case feature_set_mastering_display_primaries:
        mFeatures |= Feature::SetMasteringDisplayPrimaries;
        break;
    case feature_extended_target_volume:
        mFeatures |= Feature::ExtendedTargetVolume;
        break;
    }
}

void ColorManager::wp_color_manager_v1_supported_primaries_named(uint32_t primaries)
{
    mPrimaries.push_back(QtWayland::wp_color_manager_v1::primaries(primaries));
}

void ColorManager::wp_color_manager_v1_supported_tf_named(uint32_t transferFunction)
{
    mTransferFunctions.push_back(QtWayland::wp_color_manager_v1::transfer_function(transferFunction));
}

ColorManager::Features ColorManager::supportedFeatures() const
{
    return mFeatures;
}

bool ColorManager::supportsNamedPrimary(QtWayland::wp_color_manager_v1::primaries primaries) const
{
    return mPrimaries.contains(primaries);
}

bool ColorManager::supportsTransferFunction(QtWayland::wp_color_manager_v1::transfer_function transferFunction) const
{
    return mTransferFunctions.contains(transferFunction);
}

std::unique_ptr<ImageDescription> ColorManager::createImageDescription(const QColorSpace &colorspace)
{
    if (!(mFeatures & Feature::Parametric))
        return nullptr;

    constexpr std::array primaryMapping = {
        std::make_pair(QColorSpace::Primaries::SRgb, primaries_srgb),
        std::make_pair(QColorSpace::Primaries::AdobeRgb, primaries_adobe_rgb),
        std::make_pair(QColorSpace::Primaries::DciP3D65, primaries_display_p3),
        std::make_pair(QColorSpace::Primaries::Bt2020, primaries_bt2020),
    };
    const auto primary = std::find_if(primaryMapping.begin(), primaryMapping.end(), [&colorspace](const auto &pair) {
        return pair.first == colorspace.primaries();
    });
    if (!(supportedFeatures() & Feature::SetPrimaries) && (primary == primaryMapping.end() || !supportsNamedPrimary(primary->second)))
        return nullptr;

    constexpr std::array tfMapping = {
        std::make_pair(QColorSpace::TransferFunction::Linear, transfer_function_ext_linear),
        std::make_pair(QColorSpace::TransferFunction::SRgb, transfer_function_gamma22),
        std::make_pair(QColorSpace::TransferFunction::St2084, transfer_function_st2084_pq),
        std::make_pair(QColorSpace::TransferFunction::Hlg, transfer_function_hlg),
    };
    const auto tfIt = std::find_if(tfMapping.begin(), tfMapping.end(), [&colorspace](const auto &pair) {
        return pair.first == colorspace.transferFunction();
    });
    auto transferFunction = tfIt == tfMapping.end() ? std::nullopt : std::make_optional(tfIt->second);
    if (colorspace.transferFunction() == QColorSpace::TransferFunction::Gamma) {
        if (qFuzzyCompare(colorspace.gamma(), 2.2f) && supportsTransferFunction(transfer_function_gamma22))
            transferFunction = transfer_function_gamma22;
        else if (qFuzzyCompare(colorspace.gamma(), 2.8f) && supportsTransferFunction(transfer_function_gamma28))
            transferFunction = transfer_function_gamma28;
        if (!transferFunction && !(mFeatures & Feature::PowerTransferFunction)) {
            if (qFuzzyCompare(colorspace.gamma(), 563.0f / 256.0f) && supportsTransferFunction(transfer_function_gamma22)) {
                // If power tf is not supported, we can use Adobe RGB gamma approximation
                transferFunction = transfer_function_gamma22;
            } else {
                return nullptr;
            }
        }
    } else if (!transferFunction || !supportsTransferFunction(*transferFunction)) {
        return nullptr;
    }

    auto creator = create_parametric_creator();
    if (primary != primaryMapping.end()) {
        wp_image_description_creator_params_v1_set_primaries_named(creator, primary->second);
    } else {
        const auto primaries = colorspace.primaryPoints();
        wp_image_description_creator_params_v1_set_primaries(creator,
            std::round(1'000'000 * primaries.redPoint.x()), std::round(1'000'000 * primaries.redPoint.y()),
            std::round(1'000'000 * primaries.greenPoint.x()), std::round(1'000'000 * primaries.greenPoint.y()),
            std::round(1'000'000 * primaries.bluePoint.x()), std::round(1'000'000 * primaries.bluePoint.y()),
            std::round(1'000'000 * primaries.whitePoint.x()), std::round(1'000'000 * primaries.whitePoint.y())
        );
    }
    if (transferFunction) {
        wp_image_description_creator_params_v1_set_tf_named(creator, *transferFunction);
    } else {
        Q_ASSERT(colorspace.transferFunction() == QColorSpace::TransferFunction::Gamma);
        wp_image_description_creator_params_v1_set_tf_power(creator, std::round(colorspace.gamma() * 10'000));
    }
    return std::make_unique<ImageDescription>(wp_image_description_creator_params_v1_create(creator));
}

ImageDescriptionInfo::ImageDescriptionInfo(ImageDescription *descr)
    : QtWayland::wp_image_description_info_v1(descr->get_information())
{
}

ImageDescriptionInfo::~ImageDescriptionInfo()
{
    wp_image_description_info_v1_destroy(object());
}

void ImageDescriptionInfo::wp_image_description_info_v1_done()
{
    Q_EMIT done();
}

void ImageDescriptionInfo::wp_image_description_info_v1_icc_file(int32_t icc, uint32_t icc_size)
{
    Q_UNUSED(icc_size)
    close(icc);
}

void ImageDescriptionInfo::wp_image_description_info_v1_primaries(int32_t r_x, int32_t r_y, int32_t g_x, int32_t g_y, int32_t b_x, int32_t b_y, int32_t w_x, int32_t w_y)
{
    mContainerRed = QPointF(r_x, r_y) / 1'000'000.0;
    mContainerGreen = QPointF(g_x, g_y) / 1'000'000.0;
    mContainerBlue = QPointF(b_x, b_y) / 1'000'000.0;
    mContainerWhite = QPointF(w_x, w_y) / 1'000'000.0;
}

void ImageDescriptionInfo::wp_image_description_info_v1_tf_named(uint32_t transferFunction)
{
    mTransferFunction = transferFunction;
}

void ImageDescriptionInfo::wp_image_description_info_v1_luminances(uint32_t min_lum, uint32_t max_lum, uint32_t reference_lum)
{
    mMinLuminance = min_lum / 10'000.0;
    mMaxLuminance = max_lum;
    mReferenceLuminance = reference_lum;
}

void ImageDescriptionInfo::wp_image_description_info_v1_target_primaries(int32_t r_x, int32_t r_y, int32_t g_x, int32_t g_y, int32_t b_x, int32_t b_y, int32_t w_x, int32_t w_y)
{
    mTargetRed = QPointF(r_x, r_y) / 1'000'000.0;
    mTargetGreen = QPointF(g_x, g_y) / 1'000'000.0;
    mTargetBlue = QPointF(b_x, b_y) / 1'000'000.0;
    mTargetWhite = QPointF(w_x, w_y) / 1'000'000.0;
}

void ImageDescriptionInfo::wp_image_description_info_v1_target_luminance(uint32_t min_lum, uint32_t max_lum)
{
    mTargetMinLuminance = min_lum / 10'000.0;
    mTargetMaxLuminance = max_lum;
}

ImageDescription::ImageDescription(::wp_image_description_v1 *descr)
    : QtWayland::wp_image_description_v1(descr)
{
}

ImageDescription::~ImageDescription()
{
    wp_image_description_v1_destroy(object());
}

void ImageDescription::wp_image_description_v1_failed(uint32_t cause, const QString &msg)
{
    Q_UNUSED(cause);
    qCWarning(lcQpaWayland) << "image description failed!" << msg;
    // TODO handle this, somehow
    // maybe fall back to the previous or preferred image description
}

void ImageDescription::wp_image_description_v1_ready(uint32_t identity)
{
    Q_UNUSED(identity);
    Q_EMIT ready();
}

ColorManagementFeedback::ColorManagementFeedback(::wp_color_management_surface_feedback_v1 *obj)
    : QtWayland::wp_color_management_surface_feedback_v1(obj)
    , mPreferred(std::make_unique<ImageDescription>(get_preferred()))
{
}

ColorManagementFeedback::~ColorManagementFeedback()
{
    wp_color_management_surface_feedback_v1_destroy(object());
}

void ColorManagementFeedback::wp_color_management_surface_feedback_v1_preferred_changed(uint32_t identity)
{
    Q_UNUSED(identity);
    mPreferred = std::make_unique<ImageDescription>(get_preferred());
    mPendingPreferredInfo = std::make_unique<ImageDescriptionInfo>(mPreferred.get());
    connect(mPendingPreferredInfo.get(), &ImageDescriptionInfo::done, this, &ColorManagementFeedback::preferredChanged);
}

void ColorManagementFeedback::handlePreferredDone()
{
    mPreferredInfo = std::move(mPendingPreferredInfo);
}

ColorManagementSurface::ColorManagementSurface(::wp_color_management_surface_v1 *obj)
    : QtWayland::wp_color_management_surface_v1(obj)
{
}

ColorManagementSurface::~ColorManagementSurface()
{
    wp_color_management_surface_v1_destroy(object());
}

void ColorManagementSurface::setImageDescription(ImageDescription *descr)
{
    if (descr)
        wp_color_management_surface_v1_set_image_description(object(), descr->object(), QtWayland::wp_color_manager_v1::render_intent::render_intent_perceptual);
    else
        wp_color_management_surface_v1_unset_image_description(object());
}

}

QT_END_NAMESPACE

#include "moc_qwaylandcolormanagement_p.cpp"
