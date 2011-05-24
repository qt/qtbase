/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

// Implements two dithering methods:
//
//  * convertRGBA32_to_RGB565
//
// This is implemented using Ordered Bayer Dithering. The code has been adapted
// from QX11PixmapData::fromImage. This method was originally implemented using
// Floyd-Steinberg dithering but was later changed to Ordered Dithering because
// of the better quality of the results.
//
//  * convertRGBA32_to_RGBA4444
//
// This is implemented using Floyd-Steinberg dithering.
//
// The alghorithm used here is not the fastest possible but it's prolly fast enough:
// uses look-up tables, integer-only arthmetics and works in one pass on two lines
// at a time. It's a high-quality dithering using 1/8 diffusion precission.
// Each channel (RGBA) is diffused independently and alpha is dithered too.

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <QVarLengthArray>

// Gets a component (red = 1, green = 2...) from a RGBA data structure.
// data is unsigned char. stride is the number of bytes per line.
#define GET_RGBA_COMPONENT(data, x, y, stride, c) (data[(y * stride) + (x << 2) + c])

// Writes a new pixel with r, g, b to data in 565 16bit format. Data is a short.
#define PUT_565(data, x, y, width, r, g, b) (data[(y * width) + x] = (r << 11) | (g << 5) | b)

// Writes a new pixel with r, g, b, a to data in 4444 RGBA 16bit format. Data is a short.
#define PUT_4444(data, x, y, width, r, g, b, a) (data[(y * width) + x] = (r << 12) | (g << 8) | (b << 4) | a)

// Writes(ads) a new value to the diffusion accumulator. accumulator is a short.
// x, y is a position in the accumulation buffer. y can be 0 or 1 -- we operate on two lines at time.
#define ACCUMULATE(accumulator, x, y, width, v) if (x < width && x >= 0) accumulator[(y * width) + x] += v

// Clamps a value to be in 0..255 range.
#define CLAMP_256(v) if (v > 255) v = 255; if (v < 0) v = 0;

// Converts incoming RGB32 (QImage::Format_RGB32) to RGB565. Returns the newly allocated data.
unsigned short* convertRGB32_to_RGB565(const unsigned char *in, int width, int height, int stride)
{
    static bool thresholdMapInitialized = false;
    static int thresholdMap[16][16];

    if (!thresholdMapInitialized) {
        int i;
        int j;
        int n;

        thresholdMap[0][0] = 0;
        thresholdMap[1][0] = 2;
        thresholdMap[0][1] = 3;
        thresholdMap[1][1] = 1;

        for (n=2; n<16; n*=2) {
            for (i=0; i<n; i++) {
                for (j=0; j<n; j++) {
                    thresholdMap[i][j]    *= 4;
                    thresholdMap[i+n][j]   = thresholdMap[i][j] + 2;
                    thresholdMap[i][j+n]   = thresholdMap[i][j] + 3;
                    thresholdMap[i+n][j+n] = thresholdMap[i][j] + 1;
                }
            }
        }

        thresholdMapInitialized = true;
    }

    // Output line stride. Aligned to 4 bytes.
    int alignedWidth = width;
    if (alignedWidth % 2 > 0)
        alignedWidth++;

    // Will store output
    unsigned short *out = (unsigned short *)malloc (alignedWidth * height * 2);

    int x;
    int y;
    int threshold;

    // For each line...
    for (y = 0; y < height; y++) {

        // For each column....
        for (x = 0; x < width; x++) {

            int r = GET_RGBA_COMPONENT(in, x, y, stride, 0);
            int g = GET_RGBA_COMPONENT(in, x, y, stride, 1);
            int b = GET_RGBA_COMPONENT(in, x, y, stride, 2);

            threshold = thresholdMap[x%16][y%16];

            if (r <= (255-(1<<3)) && ((r<<5) & 255) > threshold) r += (1<<3);
            if (g <= (255-(1<<2)) && ((g<<6) & 255) > threshold) g += (1<<2);
            if (b <= (255-(1<<3)) && ((b<<5) & 255) > threshold) b += (1<<3);

            // Write the newly produced pixel
            PUT_565(out, x, y, alignedWidth, ((b >> 3) & 0x1f), ((g >> 2) & 0x3f), ((r >> 3) & 0x1f));
        }
    }

    return out;
}

// Converts incoming RGBA32 (QImage::Format_ARGB32_Premultiplied) to RGB565. Returns the newly allocated data.
// This function is similar (yet different) to the _565 variant but it makes sense to duplicate it here for simplicity.
// The output has each scan line aligned to 4 bytes (as expected by GL by default).
unsigned short* convertARGB32_to_RGBA4444(const unsigned char *in, int width, int height, int stride)
{
    // Output line stride. Aligned to 4 bytes.
    int alignedWidth = width;
    if (alignedWidth % 2 > 0)
        alignedWidth++;

    // Will store output
    unsigned short *out = (unsigned short *) malloc(alignedWidth * 2 * height);

    // Lookup tables for the 8bit => 4bit conversion
    unsigned char lookup_8bit_to_4bit[256];
    short lookup_8bit_to_4bit_diff[256];

    // Macros for the conversion using the lookup table.
    #define CONVERT_8BIT_TO_4BIT(v) (lookup_8bit_to_4bit[v])
    #define DIFF_8BIT_TO_4BIT(v) (lookup_8bit_to_4bit_diff[v])

    int i;
    int x, y, c; // Pixel we're processing. c is component number (0, 1, 2, 3 for r, b, b, a)
    short component[4]; // Stores the new components (r, g, b, a) for pixel produced during conversion
    short diff; // The difference between the converted value and the original one. To be accumulated.
    QVarLengthArray <short> accumulatorData(4 * width * 2); // Data for three acumulators for r, g, b. Each accumulator is two lines.
    short *accumulator[4]; // Helper for accessing the accumulator on a per-channel basis more easily.
    accumulator[0] = accumulatorData.data();
    accumulator[1] = accumulatorData.data() + width;
    accumulator[2] = accumulatorData.data() + (width * 2);
    accumulator[3] = accumulatorData.data() + (width * 3);

    // Produce the conversion lookup tables.
    for (i = 0; i < 256; i++) {
        lookup_8bit_to_4bit[i] = round(i / 16.0);
        // Before bitshifts: (i * 8) - (... * 16 * 8)
        lookup_8bit_to_4bit_diff[i] = (i << 3) - (lookup_8bit_to_4bit[i] << 7);

        if (lookup_8bit_to_4bit[i] > 15)
            lookup_8bit_to_4bit[i] = 15;
    }

    // Clear the accumulators
    memset(accumulator[0], 0, width * 4);
    memset(accumulator[1], 0, width * 4);
    memset(accumulator[2], 0, width * 4);
    memset(accumulator[3], 0, width * 4);

    // For each line...
    for (y = 0; y < height; y++) {

        // For each component (r, g, b, a)...
        memcpy(accumulator[0], accumulator[0] + width, width * 2);
        memset(accumulator[0] + width, 0, width * 2);

        memcpy(accumulator[1], accumulator[1] + width, width * 2);
        memset(accumulator[1] + width, 0, width * 2);

        memcpy(accumulator[2], accumulator[2] + width, width * 2);
        memset(accumulator[2] + width, 0, width * 2);

        memcpy(accumulator[3], accumulator[3] + width, width * 2);
        memset(accumulator[3] + width, 0, width * 2);

        // For each column....
        for (x = 0; x < width; x++) {

            // For each component (r, g, b, a)...
            for (c = 0; c < 4; c++) {

                // Get the 8bit value from the original image
                component[c] = GET_RGBA_COMPONENT(in, x, y, stride, c);

                // Add the diffusion for this pixel we stored in the accumulator.
                // >> 7 because the values in accumulator are stored * 128
                component[c] += accumulator[c][x] >> 7;

                // Make sure we're not over the boundaries.
                CLAMP_256(component[c]);

                // Store the difference from converting 8bit => 4bit and the orig pixel.
                // Convert 8bit => 4bit.
                diff = DIFF_8BIT_TO_4BIT(component[c]);
                component[c] = CONVERT_8BIT_TO_4BIT(component[c]);

                // Distribute the difference according to the matrix in the
                // accumulation bufffer.
                ACCUMULATE(accumulator[c], x + 1, 0, width, diff * 7);
                ACCUMULATE(accumulator[c], x - 1, 1, width, diff * 3);
                ACCUMULATE(accumulator[c], x, 1, width, diff * 5);
                ACCUMULATE(accumulator[c], x + 1, 1, width, diff * 1);
            }

            // Write the newly produced pixel
            PUT_4444(out, x, y, alignedWidth, component[0], component[1], component[2], component[3]);
        }
    }

    return out;
}

unsigned char* convertBGRA32_to_RGBA32(const unsigned char *in, int width, int height, int stride)
{
    unsigned char *out = (unsigned char *) malloc(stride * height);

    // For each line...
    for (int y = 0; y < height; y++) {
        // For each column
        for (int x = 0; x < width; x++) {
            out[(stride * y) + (x * 4) + 0] = in[(stride * y) + (x * 4) + 2];
            out[(stride * y) + (x * 4) + 1] = in[(stride * y) + (x * 4) + 1];
            out[(stride * y) + (x * 4) + 2] = in[(stride * y) + (x * 4) + 0];
            out[(stride * y) + (x * 4) + 3] = in[(stride * y) + (x * 4) + 3];
        }
    }

    return out;
}
