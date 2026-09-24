// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qglobal.h>
#include <QtCore/qsize.h>

#import <Metal/Metal.h>

// Creates a rasterization rate map with the given screen size and a 2x2
// quality grid that halves the resolution in the second row and column, so
// that the physical size ends up smaller than the screen size.
quint64 tst_qrhi_createMetalRasterizationRateMap(void *device, const QSize &screenSize)
{
    id<MTLDevice> dev = (id<MTLDevice>) device;
    if (![dev supportsRasterizationRateMapWithLayerCount: 1])
        return 0;

    id<MTLRasterizationRateMap> rateMap = nil;
    @autoreleasepool {
        const float horizontal[] = { 1.0f, 0.5f };
        const float vertical[] = { 1.0f, 0.5f };
        MTLRasterizationRateLayerDescriptor *layer =
            [[MTLRasterizationRateLayerDescriptor alloc] initWithSampleCount: MTLSizeMake(2, 2, 1)
                                                                  horizontal: horizontal
                                                                    vertical: vertical];
        MTLRasterizationRateMapDescriptor *desc =
            [MTLRasterizationRateMapDescriptor rasterizationRateMapDescriptorWithScreenSize: MTLSizeMake(screenSize.width(), screenSize.height(), 1)
                                                                                     layer: layer];
        rateMap = [dev newRasterizationRateMapWithDescriptor: desc];
        [layer release];
    }
    return quint64(rateMap);
}

void tst_qrhi_releaseMetalObject(quint64 object)
{
    [(id) object release];
}
