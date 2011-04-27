/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qscreenqnx_qws.h"
#include "qdebug.h"

#include <gf/gf.h>

QT_BEGIN_NAMESPACE

// This struct holds all the pointers to QNX's internals
struct QQnxScreenContext
{
    inline QQnxScreenContext()
        : device(0), display(0), layer(0), hwSurface(0), memSurface(0), context(0)
    {}

    gf_dev_t device;
    gf_dev_info_t deviceInfo;
    gf_display_t display;
    gf_display_info_t displayInfo;
    gf_layer_t layer;
    gf_surface_t hwSurface;
    gf_surface_t memSurface;
    gf_surface_info_t memSurfaceInfo;
    gf_context_t context;
};

/*!
    \class QQnxScreen
    \preliminary
    \ingroup qws
    \since 4.6
    \internal

    \brief The QQnxScreen class implements a screen driver
    for QNX io-display based devices.

    Note - you never have to instanciate this class, the QScreenDriverFactory
    does that for us based on the \c{QWS_DISPLAY} environment variable.

    To activate this driver, set \c{QWS_DISPLAY} to \c{qnx}.

    Example:
    \c{QWS_DISPLAY=qnx; export QWS_DISPLAY}

    By default, the main layer of the first display of the first device is used.
    If you have multiple graphic cards, multiple displays or multiple layers and
    don't want to connect to the default, you can override that with setting
    the corresponding options \c{device}, \c{display} or \c{layer} in the \c{QWS_DISPLAY} variable:

    \c{QWS_DISPLAY=qnx:device=3:display=4:layer=5}

    In addition, it is suggested to set the physical width and height of the display.
    QQnxScreen will use that information to compute the dots per inch (DPI) in order to render
    fonts correctly. If this informaiton is omitted, QQnxScreen defaults to 72 dpi.

    \c{QWS_DISPLAY=qnx:mmWidth=120:mmHeight=80}

    \c{mmWidth} and \c{mmHeight} are the physical width/height of the screen in millimeters.

    \sa QScreen, QScreenDriverPlugin, {Running Qt for Embedded Linux Applications}{Running Applications}
*/

/*!
    Constructs a QQnxScreen object. The \a display_id argument
    identifies the Qt for Embedded Linux server to connect to.
*/
QQnxScreen::QQnxScreen(int display_id)
    : QScreen(display_id), d(new QQnxScreenContext)
{
}

/*!
    Destroys this QQnxScreen object.
*/
QQnxScreen::~QQnxScreen()
{
    delete d;
}

/*! \reimp
*/
bool QQnxScreen::initDevice()
{
    // implement this if you have multiple processes that want to access the display
    // (not required if QT_NO_QWS_MULTIPROCESS is set)
    return true;
}

/*! \internal
  Attaches to the named device \a name.
*/
static bool attachDevice(QQnxScreenContext * const d, const char *name)
{
    int ret = gf_dev_attach(&d->device, name, &d->deviceInfo);
    if (ret != GF_ERR_OK) {
        qWarning("QQnxScreen: gf_dev_attach(%s) failed with error code %d", name, ret);
        return false;
    }
    return true;
}

/*! \internal
  Attaches to the display at index \a displayIndex.
 */
static bool attachDisplay(QQnxScreenContext * const d, int displayIndex)
{
    int ret = gf_display_attach(&d->display, d->device, displayIndex, &d->displayInfo);
    if (ret != GF_ERR_OK) {
        qWarning("QQnxScreen: gf_display_attach(%d) failed with error code %d",
                displayIndex, ret);
        return false;
    }
    return true;
}

/*! \internal
  Attaches to the layer \a layerIndex.
 */
static bool attachLayer(QQnxScreenContext * const d, int layerIndex)
{
    int ret = gf_layer_attach(&d->layer, d->display, layerIndex, 0);
    if (ret != GF_ERR_OK) {
        qWarning("QQnxScreen: gf_layer_attach(%d) failed with error code %d", layerIndex,
                ret);
        return false;
    }
    gf_layer_enable(d->layer);

    return true;
}

/*! \internal
  Creates a new hardware surface (usually on the Gfx card memory) with the dimensions \a w * \a h.
 */
static bool createHwSurface(QQnxScreenContext * const d, int w, int h)
{
    int ret = gf_surface_create_layer(&d->hwSurface, &d->layer, 1, 0,
                w, h, GF_FORMAT_ARGB8888, 0, 0);
    if (ret != GF_ERR_OK) {
        qWarning("QQnxScreen: gf_surface_create_layer(%dx%d) failed with error code %d",
                w, h, ret);
        return false;
    }

    gf_layer_set_surfaces(d->layer, &d->hwSurface, 1);

    ret = gf_layer_update(d->layer, 0);
    if (ret != GF_ERR_OK) {
        qWarning("QQnxScreen: gf_layer_update() failed with error code %d\n", ret);
        return false;
    }

    return true;
}

/*! \internal
  Creates an in-memory, linear accessible surface of dimensions \a w * \a h.
  This is the main surface that QWS blits to.
 */
static bool createMemSurface(QQnxScreenContext * const d, int w, int h)
{
    // Note: gf_surface_attach() could also be used, so we'll create the buffer
    // and let the surface point to it. Here, we use surface_create instead.

    int ret = gf_surface_create(&d->memSurface, d->device, w, h,
                GF_FORMAT_ARGB8888, 0,
                GF_SURFACE_CREATE_CPU_FAST_ACCESS | GF_SURFACE_CREATE_CPU_LINEAR_ACCESSIBLE
                | GF_SURFACE_PHYS_CONTIG | GF_SURFACE_CREATE_SHAREABLE);
    if (ret != GF_ERR_OK) {
        qWarning("QQnxScreen: gf_surface_create(%dx%d) failed with error code %d",
                w, h, ret);
        return false;
    }

    gf_surface_get_info(d->memSurface, &d->memSurfaceInfo);

    if (d->memSurfaceInfo.sid == unsigned(GF_SID_INVALID)) {
        qWarning("QQnxScreen: gf_surface_get_info() failed.");
        return false;
    }

    return true;
}

/* \internal
   Creates a QNX gf context and sets our memory surface on it.
 */
static bool createContext(QQnxScreenContext * const d)
{
    int ret = gf_context_create(&d->context);
    if (ret != GF_ERR_OK) {
        qWarning("QQnxScreen: gf_context_create() failed with error code %d", ret);
        return false;
    }

    ret = gf_context_set_surface(d->context, d->memSurface);
    if (ret != GF_ERR_OK) {
        qWarning("QQnxScreen: gf_context_set_surface() failed with error code %d", ret);
        return false;
    }

    return true;
}

/*! \reimp
  Connects to QNX's io-display based device based on the \a displaySpec parameters
  from the \c{QWS_DISPLAY} environment variable. See the QQnxScreen class documentation
  for possible parameters.

  \sa QQnxScreen
 */
bool QQnxScreen::connect(const QString &displaySpec)
{
    const QStringList params = displaySpec.split(QLatin1Char(':'), QString::SkipEmptyParts);

    bool isOk = false;
    QRegExp deviceRegExp(QLatin1String("^device=(.+)$"));
    if (params.indexOf(deviceRegExp) != -1) {
        isOk = attachDevice(d, deviceRegExp.cap(1).toLocal8Bit().constData());
    } else {
        // no device specified - attach to device 0 (the default)
        isOk = attachDevice(d, GF_DEVICE_INDEX(0));
    }

    if (!isOk)
        return false;

    qDebug("QQnxScreen: Attached to Device, number of displays: %d", d->deviceInfo.ndisplays);

    // default to display 0
    int displayIndex = 0;
    QRegExp displayRegexp(QLatin1String("^display=(\\d+)$"));
    if (params.indexOf(displayRegexp) != -1) {
        displayIndex = displayRegexp.cap(1).toInt();
    }

    if (!attachDisplay(d, displayIndex))
        return false;

    qDebug("QQnxScreen: Attached to Display %d, resolution %dx%d, refresh %d Hz",
            displayIndex, d->displayInfo.xres, d->displayInfo.yres,
            d->displayInfo.refresh);


    // default to main_layer_index from the displayInfo struct
    int layerIndex = 0;
    QRegExp layerRegexp(QLatin1String("^layer=(\\d+)$"));
    if (params.indexOf(layerRegexp) != -1) {
        layerIndex = layerRegexp.cap(1).toInt();
    } else {
        layerIndex = d->displayInfo.main_layer_index;
    }

    if (!attachLayer(d, layerIndex))
        return false;

    // tell QWSDisplay the width and height of the display
    w = dw = d->displayInfo.xres;
    h = dh = d->displayInfo.yres;

    // we only support 32 bit displays for now.
    QScreen::d = 32;

    // assume 72 dpi as default, to calculate the physical dimensions if not specified
    const int defaultDpi = 72;

    // Handle display physical size spec.
    QRegExp mmWidthRegexp(QLatin1String("^mmWidth=(\\d+)$"));
    if (params.indexOf(mmWidthRegexp) == -1) {
        physWidth = qRound(dw * 25.4 / defaultDpi);
    } else {
        physWidth = mmWidthRegexp.cap(1).toInt();
    }

    QRegExp mmHeightRegexp(QLatin1String("^mmHeight=(\\d+)$"));
    if (params.indexOf(mmHeightRegexp) == -1) {
        physHeight = qRound(dh * 25.4 / defaultDpi);
    } else {
        physHeight = mmHeightRegexp.cap(1).toInt();
    }

    // create a hardware surface with our dimensions. In the old days, it was possible
    // to get a pointer directly to the hw surface, so we could blit directly. Now, we
    // have to use one indirection more, because it's not guaranteed that the hw surface
    // is mappable into our process.
    if (!createHwSurface(d, w, h))
        return false;

    // create an in-memory linear surface that is used by QWS. QWS will blit directly in here.
    if (!createMemSurface(d, w, h))
        return false;

    // set the address of the in-memory buffer that QWS is blitting to
    data = d->memSurfaceInfo.vaddr;
    // set the line stepping
    lstep = d->memSurfaceInfo.stride;

    // the overall size of the in-memory buffer is linestep * height
    size = mapsize = lstep * h;

    // create a QNX drawing context
    if (!createContext(d))
        return false;

    // we're always using a software cursor for now. Initialize it here.
    QScreenCursor::initSoftwareCursor();

    // done, the driver should be connected to the display now.
    return true;
}

/*! \reimp
 */
void QQnxScreen::disconnect()
{
    if (d->context)
        gf_context_free(d->context);

    if (d->memSurface)
        gf_surface_free(d->memSurface);

    if (d->hwSurface)
        gf_surface_free(d->hwSurface);

    if (d->layer)
        gf_layer_detach(d->layer);

    if (d->display)
        gf_display_detach(d->display);

    if (d->device)
        gf_dev_detach(d->device);

    d->memSurface = 0;
    d->hwSurface = 0;
    d->context = 0;
    d->layer = 0;
    d->display = 0;
    d->device = 0;
}

/*! \reimp
 */
void QQnxScreen::shutdownDevice()
{
}


/*! \reimp
  QQnxScreen doesn't support setting the mode, use io-display instead.
 */
void QQnxScreen::setMode(int,int,int)
{
    qWarning("QQnxScreen: Unable to change mode, use io-display instead.");
}

/*! \reimp
 */
bool QQnxScreen::supportsDepth(int depth) const
{
    // only 32-bit for the moment
    return depth == 32;
}

/*! \reimp
 */
void QQnxScreen::exposeRegion(QRegion r, int changing)
{
    // here is where the actual magic happens. QWS will call exposeRegion whenever
    // a region on the screen is dirty and needs to be updated on the actual screen.

    // first, call the parent implementation. The parent implementation will update
    // the region on our in-memory surface
    QScreen::exposeRegion(r, changing);

    // now our in-memory surface should be up to date with the latest changes.
    // the code below copies the region from the in-memory surface to the hardware.

    // just get the bounding rectangle of the region. Most screen updates are rectangular
    // anyways. Code could be optimized to blit each and every member of the region
    // individually, but in real life, the speed-up is neglectable
    const QRect br = r.boundingRect();
    if (br.isEmpty())
        return; // ignore empty regions because gf_draw_blit2 doesn't like 0x0 dimensions

    // start drawing.
    int ret = gf_draw_begin(d->context);
    if (ret != GF_ERR_OK) {
        qWarning("QQnxScreen: gf_draw_begin() failed with error code %d", ret);
        return;
    }

    // blit the changed region from the memory surface to the hardware surface
    ret = gf_draw_blit2(d->context, d->memSurface, d->hwSurface,
            br.x(), br.y(), br.right(), br.bottom(), br.x(), br.y());
    if (ret != GF_ERR_OK) {
        qWarning("QQnxScreen: gf_draw_blit2() failed with error code %d", ret);
    }

    // flush all drawing commands (in our case, a single blit)
    ret = gf_draw_flush(d->context);
    if (ret != GF_ERR_OK) {
        qWarning("QQnxScreen: gf_draw_flush() failed with error code %d", ret);
    }

    // tell QNX that we're done drawing.
    gf_draw_end(d->context);
}

QT_END_NAMESPACE
