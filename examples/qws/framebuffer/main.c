/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
char *frameBuffer = 0;
int fbFd = 0;
int ttyFd = 0;

void printFixedInfo()
{
    printf("Fixed screen info:\n"
	   "\tid:          %s\n"
	   "\tsmem_start:  0x%lx\n"
	   "\tsmem_len:    %d\n"
	   "\ttype:        %d\n"
	   "\ttype_aux:    %d\n"
	   "\tvisual:      %d\n"
	   "\txpanstep:    %d\n"
	   "\typanstep:    %d\n"
	   "\tywrapstep:   %d\n"
	   "\tline_length: %d\n"
	   "\tmmio_start:  0x%lx\n"
	   "\tmmio_len:    %d\n"
	   "\taccel:       %d\n"
	   "\n",
	   finfo.id, finfo.smem_start, finfo.smem_len, finfo.type,
	   finfo.type_aux, finfo.visual, finfo.xpanstep, finfo.ypanstep,
	   finfo.ywrapstep, finfo.line_length, finfo.mmio_start,
	   finfo.mmio_len, finfo.accel);
}

void printVariableInfo()
{
    printf("Variable screen info:\n"
	   "\txres:           %d\n"
	   "\tyres:           %d\n"
	   "\txres_virtual:   %d\n"
	   "\tyres_virtual:   %d\n"
	   "\tyoffset:        %d\n"
	   "\txoffset:        %d\n"
	   "\tbits_per_pixel: %d\n"
	   "\tgrayscale: %d\n"
	   "\tred:    offset: %2d, length: %2d, msb_right: %2d\n"
	   "\tgreen:  offset: %2d, length: %2d, msb_right: %2d\n"
	   "\tblue:   offset: %2d, length: %2d, msb_right: %2d\n"
	   "\ttransp: offset: %2d, length: %2d, msb_right: %2d\n"
	   "\tnonstd:       %d\n"
	   "\tactivate:     %d\n"
	   "\theight:       %d\n"
	   "\twidth:        %d\n"
	   "\taccel_flags:  0x%x\n"
	   "\tpixclock:     %d\n"
	   "\tleft_margin:  %d\n"
	   "\tright_margin: %d\n"
	   "\tupper_margin: %d\n"
	   "\tlower_margin: %d\n"
	   "\thsync_len:    %d\n"
	   "\tvsync_len:    %d\n"
	   "\tsync:         %d\n"
	   "\tvmode:        %d\n"
	   "\n",
	   vinfo.xres, vinfo.yres, vinfo.xres_virtual, vinfo.yres_virtual,
	   vinfo.xoffset, vinfo.yoffset, vinfo.bits_per_pixel, vinfo.grayscale,
	   vinfo.red.offset, vinfo.red.length, vinfo.red.msb_right,
	   vinfo.green.offset, vinfo.green.length, vinfo.green.msb_right,
	   vinfo.blue.offset, vinfo.blue.length, vinfo.blue.msb_right,
	   vinfo.transp.offset, vinfo.transp.length, vinfo.transp.msb_right,
	   vinfo.nonstd, vinfo.activate, vinfo.height, vinfo.width,
	   vinfo.accel_flags, vinfo.pixclock, vinfo.left_margin,
	   vinfo.right_margin, vinfo.upper_margin, vinfo.lower_margin,
	   vinfo.hsync_len, vinfo.vsync_len, vinfo.sync, vinfo.vmode);
}

long switchToGraphicsMode()
{
    const char *const devs[] = {"/dev/tty0", "/dev/tty", "/dev/console", 0};
    const char * const *dev;
    long oldMode = KD_TEXT;

    for (dev = devs; *dev; ++dev) {
        ttyFd = open(*dev, O_RDWR);
        if (ttyFd != -1)
            break;
        printf("Opening tty device %s failed: %s\n", *dev, strerror(errno));
    }

    ioctl(ttyFd, KDGETMODE, &oldMode);
    if (oldMode == KD_GRAPHICS) {
        printf("Was in graphics mode already. Skipping\n");
        return oldMode;
    }
    int ret = ioctl(ttyFd, KDSETMODE, KD_GRAPHICS);
    if (ret == -1) {
        printf("Switch to graphics mode failed: %s\n", strerror(errno));
	return oldMode;
    }

    printf("Successfully switched to graphics mode.\n\n");

    return oldMode;
}

void restoreTextMode(long oldMode)
{
    if (ttyFd == -1)
        return;

    ioctl(ttyFd, KDSETMODE, oldMode);
    close(ttyFd);
}

struct fb_cmap oldPalette;
struct fb_cmap palette;
int paletteSize = 0;

void initPalette_16()
{
    if (finfo.type == FB_TYPE_PACKED_PIXELS) {
	// We'll setup a grayscale map for 4bpp linear
	int val = 0;
        int i;
	for (i = 0; i < 16; ++i) {
	    palette.red[i] = (val << 8) | val;
	    palette.green[i] = (val << 8) | val;
	    palette.blue[i] = (val << 8) | val;
            val += 17;
	}
	return;
    }

    // Default 16 colour palette
    unsigned char reds[16]   = { 0x00, 0x7F, 0xBF, 0xFF, 0xFF, 0xA2,
				 0x00, 0xFF, 0xFF, 0x00, 0x7F, 0x7F,
				 0x00, 0x00, 0x00, 0x82 };
    unsigned char greens[16] = { 0x00, 0x7F, 0xBF, 0xFF, 0x00, 0xC5,
				 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00,
				 0x00, 0x7F, 0x7F, 0x7F };
    unsigned char blues[16]  = { 0x00, 0x7F, 0xBF, 0xFF, 0x00, 0x11,
				 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x7F,
				 0x7F, 0x7F, 0x00, 0x00 };

    int i;
    for (i = 0; i < 16; ++i) {
	palette.red[i] = ((reds[i]) << 8) | reds[i];
	palette.green[i] = ((greens[i]) << 8) | greens[i];
	palette.blue[i] = ((blues[i]) << 8) | blues[i];
	palette.transp[i] = 0;
    }
}

void initPalette_256()
{
    if (vinfo.grayscale) {
        int i;
	for (i = 0; i < 256; ++i) {
	    unsigned short c = (i << 8) | i;
	    palette.red[i] = c;
	    palette.green[i] = c;
	    palette.blue[i] = c;
	    palette.transp[i] = 0;
	}
	return;
    }

    // 6x6x6 216 color cube
    int i = 0;
    int ir, ig, ib;
    for (ir = 0x0; ir <= 0xff; ir += 0x33) {
	for (ig = 0x0; ig <= 0xff; ig += 0x33) {
	    for (ib = 0x0; ib <= 0xff; ib += 0x33) {
		palette.red[i] = (ir << 8)|ir;
		palette.green[i] = (ig << 8)|ig;
		palette.blue[i] = (ib << 8)|ib;
		palette.transp[i] = 0;
		++i;
	    }
	}
    }
}

void initPalette()
{
    switch (vinfo.bits_per_pixel) {
    case 8: paletteSize = 256; break;
    case 4: paletteSize = 16; break;
    default: break;
    }

    if (!paletteSize)
	return; /* not using a palette */

    /* read old palette */
    oldPalette.start = 0;
    oldPalette.len = paletteSize;
    oldPalette.red = (unsigned short*)malloc(sizeof(unsigned short)*paletteSize);
    oldPalette.green = (unsigned short*)malloc(sizeof(unsigned short)*paletteSize);
    oldPalette.blue=(unsigned short*)malloc(sizeof(unsigned short)*paletteSize);
    oldPalette.transp=(unsigned short*)malloc(sizeof(unsigned short)*paletteSize);
    if (ioctl(ttyFd, FBIOGETCMAP, &oldPalette) == -1)
	perror("initPalette: error reading palette");

    /* create new palette */
    palette.start = 0;
    palette.len = paletteSize;
    palette.red = (unsigned short*)malloc(sizeof(unsigned short)*paletteSize);
    palette.green = (unsigned short*)malloc(sizeof(unsigned short)*paletteSize);
    palette.blue = (unsigned short*)malloc(sizeof(unsigned short)*paletteSize);
    palette.transp = (unsigned short*)malloc(sizeof(unsigned short)*paletteSize);
    switch (paletteSize) {
    case 16: initPalette_16(); break;
    case 256: initPalette_256(); break;
    default: break;
    }

    /* set new palette */
    if (ioctl(ttyFd, FBIOPUTCMAP, &palette) == -1)
	perror("initPalette: error setting palette");
}

void resetPalette()
{
    if (paletteSize == 0)
	return;

    if (ioctl(ttyFd, FBIOPUTCMAP, &oldPalette) == -1)
	perror("resetPalette");

    free(oldPalette.red);
    free(oldPalette.green);
    free(oldPalette.blue);
    free(oldPalette.transp);

    free(palette.red);
    free(palette.green);
    free(palette.blue);
    free(palette.transp);
}

void drawRect_rgb32(int x0, int y0, int width, int height, int color)
{
    const int bytesPerPixel = 4;
    const int stride = finfo.line_length / bytesPerPixel;

    int *dest = (int*)(frameBuffer)
                + (y0 + vinfo.yoffset) * stride
                + (x0 + vinfo.xoffset);

    int x, y;
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            dest[x] = color;
        }
        dest += stride;
    }
}

void drawRect_rgb18(int x0, int y0, int width, int height, int color)
{
    const int bytesPerPixel = 3;
    const int stride = finfo.line_length - width * bytesPerPixel;
    const int red = (color & 0xff0000) >> 16;
    const int green = (color & 0xff00) >> 8;
    const int blue = (color & 0xff);
    const unsigned int packed = (blue >> 2) |
				((green >> 2) << 6) |
				((red >> 2) << 12);
    const char color18[3] = { packed & 0xff,
			      (packed & 0xff00) >> 8,
			      (packed & 0xff0000) >> 16 };

    char *dest = (char*)(frameBuffer)
		 + (y0 + vinfo.yoffset) * stride
		 + (x0 + vinfo.xoffset);

    int x, y;
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
	    *dest++ = color18[0];
	    *dest++ = color18[1];
	    *dest++ = color18[2];
        }
        dest += stride;
    }
}

void drawRect_rgb16(int x0, int y0, int width, int height, int color)
{
    const int bytesPerPixel = 2;
    const int stride = finfo.line_length / bytesPerPixel;
    const int red = (color & 0xff0000) >> (16 + 3);
    const int green = (color & 0xff00) >> (8 + 2);
    const int blue = (color & 0xff) >> 3;
    const short color16 = blue | (green << 5) | (red << (5 + 6));

    short *dest = (short*)(frameBuffer)
		  + (y0 + vinfo.yoffset) * stride
		  + (x0 + vinfo.xoffset);

    int x, y;
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            dest[x] = color16;
        }
        dest += stride;
    }
}

void drawRect_rgb15(int x0, int y0, int width, int height, int color)
{
    const int bytesPerPixel = 2;
    const int stride = finfo.line_length / bytesPerPixel;
    const int red = (color & 0xff0000) >> (16 + 3);
    const int green = (color & 0xff00) >> (8 + 3);
    const int blue = (color & 0xff) >> 3;
    const short color15 = blue | (green << 5) | (red << (5 + 5));

    short *dest = (short*)(frameBuffer)
		  + (y0 + vinfo.yoffset) * stride
		  + (x0 + vinfo.xoffset);

    int x, y;
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            dest[x] = color15;
        }
        dest += stride;
    }
}

void drawRect_palette(int x0, int y0, int width, int height, int color)
{
    const int bytesPerPixel = 1;
    const int stride = finfo.line_length / bytesPerPixel;
    const unsigned char color8 = color;

    unsigned char *dest = (unsigned char*)(frameBuffer)
                          + (y0 + vinfo.yoffset) * stride
                          + (x0 + vinfo.xoffset);

    int x, y;
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            dest[x] = color8;
        }
        dest += stride;
    }
}

void drawRect(int x0, int y0, int width, int height, int color)
{
    switch (vinfo.bits_per_pixel) {
    case 32:
        drawRect_rgb32(x0, y0, width, height, color);
        break;
    case 18:
        drawRect_rgb18(x0, y0, width, height, color);
        break;
    case 16:
        drawRect_rgb16(x0, y0, width, height, color);
        break;
    case 15:
        drawRect_rgb15(x0, y0, width, height, color);
        break;
    case 8:
	drawRect_palette(x0, y0, width, height, color);
	break;
    case 4:
	drawRect_palette(x0, y0, width, height, color);
	break;
    default:
        printf("Warning: drawRect() not implemented for color depth %i\n",
               vinfo.bits_per_pixel);
        break;
    }
}

#define PERFORMANCE_RUN_COUNT 5
void performSpeedTest(void* fb, int fbSize)
{
    int i, j, run;

    struct timeval startTime, endTime;
    unsigned long long results[PERFORMANCE_RUN_COUNT];
    unsigned long long average;

    unsigned int* testImage;

    unsigned int randData[17] = {
        0x3A428472, 0x724B84D3, 0x26B898AB, 0x7D980E3C, 0x5345A084,
        0x6779B66B, 0x791EE4B4, 0x6E8EE3CC, 0x63AF504A, 0x18A21B33,
        0x0E26EB73, 0x022F708E, 0x1740F3B0, 0x7E2C699D, 0x0E8A570B,
        0x5F2C22FB, 0x6A742130
    };

    printf("Frame Buffer Performance test...\n");

    for (run=0; run<PERFORMANCE_RUN_COUNT; ++run) {

        /* Generate test image with random(ish) data: */
        testImage = (unsigned int*) malloc(fbSize);
        j = run;
        for (i=0; i < (int)(fbSize / sizeof(int)); ++i) {
            testImage[i] = randData[j];
            j++;
            if (j >= 17)
                j = 0;
        }

        gettimeofday(&startTime, NULL);
        memcpy(fb, testImage, fbSize);
        gettimeofday(&endTime, NULL);

        long secsDiff = endTime.tv_sec - startTime.tv_sec;
        results[run] = secsDiff * 1000000 + (endTime.tv_usec - startTime.tv_usec);

        free(testImage);
    }


    average = 0;
    for (i=0; i<PERFORMANCE_RUN_COUNT; ++i)
        average += results[i];
    average = average / PERFORMANCE_RUN_COUNT;

    printf("        Average:   %llu usecs\n", average);
    printf("        Bandwidth: %.03f MByte/Sec\n", (fbSize / 1048576.0) / ((double)average / 1000000.0));
    printf("        Max. FPS:  %.03f fps\n\n", 1000000.0 / (double)average);

    /* Clear the framebuffer back to black again: */
    memset(fb, 0, fbSize);
}

int main(int argc, char **argv)
{
    long int screensize = 0;
    int doGraphicsMode = 1;
    long oldKdMode = KD_TEXT;
    const char *devfile = "/dev/fb0";
    int nextArg = 1;

    if (nextArg < argc) {
        if (strncmp("nographicsmodeswitch", argv[nextArg],
                    strlen("nographicsmodeswitch")) == 0)
        {
	    ++nextArg;
            doGraphicsMode = 0;
        }
    }
    if (nextArg < argc)
	devfile = argv[nextArg++];

    /* Open the file for reading and writing */
    fbFd = open(devfile, O_RDWR);
    if (fbFd == -1) {
	perror("Error: cannot open framebuffer device");
	exit(1);
    }
    printf("The framebuffer device was opened successfully.\n\n");

    /* Get fixed screen information */
    if (ioctl(fbFd, FBIOGET_FSCREENINFO, &finfo) == -1) {
	perror("Error reading fixed information");
	exit(2);
    }

    printFixedInfo();

    /* Figure out the size of the screen in bytes */
    screensize = finfo.smem_len;

    /* Map the device to memory */
    frameBuffer = (char *)mmap(0, screensize,
                               PROT_READ | PROT_WRITE, MAP_SHARED,
                               fbFd, 0);
    if (frameBuffer == MAP_FAILED) {
	perror("Error: Failed to map framebuffer device to memory");
	exit(4);
    }
    printf("The framebuffer device was mapped to memory successfully.\n"
           "\n");

    if (doGraphicsMode)
        oldKdMode = switchToGraphicsMode();

    /* Get variable screen information */
    if (ioctl(fbFd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
	perror("Error reading variable information");
	exit(3);
    }

    printVariableInfo();

    performSpeedTest(frameBuffer, screensize);

    initPalette();

    if (paletteSize == 0) {
        printf("Will draw 3 rectangles on the screen,\n"
               "they should be colored red, green and blue (in that order).\n");
        drawRect(vinfo.xres / 8, vinfo.yres / 8,
                 vinfo.xres / 4, vinfo.yres / 4,
                 0xffff0000);
        drawRect(vinfo.xres * 3 / 8, vinfo.yres * 3 / 8,
                 vinfo.xres / 4, vinfo.yres / 4,
                 0xff00ff00);
        drawRect(vinfo.xres * 5 / 8, vinfo.yres * 5 / 8,
                 vinfo.xres / 4, vinfo.yres / 4,
                 0xff0000ff);
    } else {
        printf("Will rectangles from the 16 first entries in the color palette"
               " on the screen\n");
        int y;
        int x;
        for (y = 0; y < 4; ++y) {
            for (x = 0; x < 4; ++x) {
                drawRect(vinfo.xres / 4 * x, vinfo.yres / 4 * y,
                         vinfo.xres / 4, vinfo.yres / 4,
                         4 * y + x);
            }
        }
    }

    sleep(5);

    resetPalette();

    printf("  Done.\n");

    if (doGraphicsMode)
        restoreTextMode(oldKdMode);

    munmap(frameBuffer, screensize);
    close(fbFd);
    return 0;
}
