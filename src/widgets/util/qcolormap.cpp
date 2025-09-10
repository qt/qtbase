// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcolormap.h"
#include "qcolor.h"
#include "qpaintdevice.h"
#include "qscreen.h"
#include "qguiapplication.h"

QT_BEGIN_NAMESPACE

class QColormapPrivate
{
public:
    inline QColormapPrivate()
        : ref(1), mode(QColormap::Direct), depth(0), numcolors(0)
    { }

    QAtomicInt ref;

    QColormap::Mode mode;
    int depth;
    int numcolors;
};

static QColormapPrivate *screenMap = nullptr;

void QColormap::initialize()
{
    screenMap = new QColormapPrivate;
    if (Q_UNLIKELY(!QGuiApplication::primaryScreen())) {
        qWarning("no screens available, assuming 24-bit color");
        screenMap->depth = 24;
        screenMap->mode = QColormap::Direct;
        return;
    }
    screenMap->depth = QGuiApplication::primaryScreen()->depth();
    if (screenMap->depth < 8) {
        screenMap->mode = QColormap::Indexed;
        screenMap->numcolors = 256;
    } else {
        screenMap->mode = QColormap::Direct;
        screenMap->numcolors = -1;
    }
}

void QColormap::cleanup()
{
    delete screenMap;
    screenMap = nullptr;
}

QColormap QColormap::instance(int /*screen*/)
{
    return QColormap();
}

QColormap::QColormap()
    : d(screenMap)
{ d->ref.ref(); }

QColormap::QColormap(const QColormap &colormap)
    :d (colormap.d)
{ d->ref.ref(); }

QColormap::~QColormap()
{
    if (!d->ref.deref())
        delete d;
}

QColormap::Mode QColormap::mode() const
{ return d->mode; }


int QColormap::depth() const
{ return d->depth; }


int QColormap::size() const
{
    return d->numcolors;
}

#ifndef QT_QWS_DEPTH16_RGB
#define QT_QWS_DEPTH16_RGB 565
#endif
static const int qt_rbits = (QT_QWS_DEPTH16_RGB/100);
static const int qt_gbits = (QT_QWS_DEPTH16_RGB/10%10);
static const int qt_bbits = (QT_QWS_DEPTH16_RGB%10);
static const int qt_red_shift = qt_bbits+qt_gbits-(8-qt_rbits);
static const int qt_green_shift = qt_bbits-(8-qt_gbits);
static const int qt_neg_blue_shift = 8-qt_bbits;
static const int qt_blue_mask = (1<<qt_bbits)-1;
static const int qt_green_mask = (1<<(qt_gbits+qt_bbits))-(1<<qt_bbits);
static const int qt_red_mask = (1<<(qt_rbits+qt_gbits+qt_bbits))-(1<<(qt_gbits+qt_bbits));

static const int qt_red_rounding_shift = qt_red_shift + qt_rbits;
static const int qt_green_rounding_shift = qt_green_shift + qt_gbits;
static const int qt_blue_rounding_shift = qt_bbits - qt_neg_blue_shift;

inline ushort qt_convRgbTo16(QRgb c)
{
    const int tr = qRed(c) << qt_red_shift;
    const int tg = qGreen(c) << qt_green_shift;
    const int tb = qBlue(c) >> qt_neg_blue_shift;

    return (tb & qt_blue_mask) | (tg & qt_green_mask) | (tr & qt_red_mask);
}

inline QRgb qt_conv16ToRgb(ushort c)
{
    const int r=(c & qt_red_mask);
    const int g=(c & qt_green_mask);
    const int b=(c & qt_blue_mask);
    const int tr = r >> qt_red_shift | r >> qt_red_rounding_shift;
    const int tg = g >> qt_green_shift | g >> qt_green_rounding_shift;
    const int tb = b << qt_neg_blue_shift | b >> qt_blue_rounding_shift;

    return qRgb(tr,tg,tb);
}

uint QColormap::pixel(const QColor &color) const
{
    QRgb rgb = color.rgba();
    if (d->mode == QColormap::Direct) {
        switch(d->depth) {
        case 16:
            return qt_convRgbTo16(rgb);
        case 24:
        case 32:
        {
            const int r = qRed(rgb);
            const int g = qGreen(rgb);
            const int b = qBlue(rgb);
            const int red_shift = 16;
            const int green_shift = 8;
            const int red_mask   = 0xff0000;
            const int green_mask = 0x00ff00;
            const int blue_mask  = 0x0000ff;
            const int tg = g << green_shift;
            const int tr = r << red_shift;
            return 0xff000000 | (b & blue_mask) | (tg & green_mask) | (tr & red_mask);
        }
        }
    }
    //XXX
    //return qt_screen->alloc(qRed(rgb), qGreen(rgb), qBlue(rgb));
    return 0;
}

const QColor QColormap::colorAt(uint pixel) const
{
    if (d->mode == Direct) {
        if (d->depth == 16) {
            pixel = qt_conv16ToRgb(pixel);
        }
        const int red_shift = 16;
        const int green_shift = 8;
        const int red_mask   = 0xff0000;
        const int green_mask = 0x00ff00;
        const int blue_mask  = 0x0000ff;
        return QColor((pixel & red_mask) >> red_shift,
                      (pixel & green_mask) >> green_shift,
                      (pixel & blue_mask));
    }
#if 0 // XXX
    Q_ASSERT_X(int(pixel) < qt_screen->numCols(), "QColormap::colorAt", "pixel out of bounds of palette");
    return QColor(qt_screen->clut()[pixel]);
#endif
    return QColor();
}

const QList<QColor> QColormap::colormap() const
{
    return QList<QColor>();
}

QColormap &QColormap::operator=(const QColormap &colormap)
{ qAtomicAssign(d, colormap.d); return *this; }

QT_END_NAMESPACE
