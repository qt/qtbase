// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <Metal/Metal.h>
#include <rhi/qrhi.h>

void *makeRateMap(QRhi *rhi, const QSize &outputSizeInPixels)
{
    // note that multiview needs two layers, this example only uses one

    MTLDevice *dev = static_cast<const QRhiMetalNativeHandles *>(rhi->nativeHandles())->dev;
    MTLRasterizationRateMapDescriptor *descriptor = [[MTLRasterizationRateMapDescriptor alloc] init];
    descriptor.screenSize = MTLSizeMake(outputSizeInPixels.width(), outputSizeInPixels.height(), 1);
    MTLSize zoneCounts = MTLSizeMake(8, 8, 1);
    MTLRasterizationRateLayerDescriptor *layerDescriptor = [[MTLRasterizationRateLayerDescriptor alloc] initWithSampleCount:zoneCounts];
    for (uint row = 0; row < zoneCounts.height; row++)
        layerDescriptor.verticalSampleStorage[row] = 1.0;
    for (uint column = 0; column < zoneCounts.width; column++)
        layerDescriptor.horizontalSampleStorage[column] = 1.0;
    layerDescriptor.horizontalSampleStorage[0] = 0.25;
    layerDescriptor.horizontalSampleStorage[7] = 0.25;
    layerDescriptor.verticalSampleStorage[0] = 0.25;
    layerDescriptor.verticalSampleStorage[7] = 0.25;
    [descriptor setLayer:layerDescriptor atIndex:0];
    id<MTLRasterizationRateMap> rateMap = [dev newRasterizationRateMapWithDescriptor: descriptor];
    return rateMap;
}

void releaseRateMap(void *map)
{
    id<MTLRasterizationRateMap> rateMap = (id<MTLRasterizationRateMap>) map;
    [rateMap release];
}
