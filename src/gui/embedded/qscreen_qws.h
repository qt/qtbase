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

#ifndef QSCREEN_QWS_H
#define QSCREEN_QWS_H

#include <QtCore/qnamespace.h>
#include <QtCore/qpoint.h>
#include <QtCore/qlist.h>
#include <QtGui/qrgb.h>
#include <QtCore/qrect.h>
#include <QtGui/qimage.h>
#include <QtGui/qregion.h>

struct fb_cmap;

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QScreenCursor;
class QBrush;
class QWSWindow;
class QWSWindowSurface;
class QGraphicsSystem;
class QPixmapData;

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


inline ushort qt_convRgbTo16(const int r, const int g, const int b)
{
    const int tr = r << qt_red_shift;
    const int tg = g << qt_green_shift;
    const int tb = b >> qt_neg_blue_shift;

    return (tb & qt_blue_mask) | (tg & qt_green_mask) | (tr & qt_red_mask);
}

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

inline void qt_conv16ToRgb(ushort c, int& r, int& g, int& b)
{
    const int tr=(c & qt_red_mask);
    const int tg=(c & qt_green_mask);
    const int tb=(c & qt_blue_mask);
    r = tr >> qt_red_shift | tr >> qt_red_rounding_shift;
    g = tg >> qt_green_shift | tg >> qt_green_rounding_shift;
    b = tb << qt_neg_blue_shift | tb >> qt_blue_rounding_shift;
}

const int SourceSolid=0;
const int SourcePixmap=1;

#ifndef QT_NO_QWS_CURSOR

class QScreenCursor;
extern QScreenCursor *qt_screencursor;
extern bool qws_sw_cursor;

class Q_GUI_EXPORT QScreenCursor
{
public:
    QScreenCursor();
    virtual ~QScreenCursor();

    virtual void set(const QImage &image, int hotx, int hoty);
    virtual void move(int x, int y);
    virtual void show();
    virtual void hide();

    bool supportsAlphaCursor() const { return supportsAlpha; }

    static bool enabled() { return qws_sw_cursor; }

    QRect boundingRect() const { return QRect(pos - hotspot, size); }
    QImage image() const { return cursor; }
    bool isVisible() const { return enable; }
    bool isAccelerated() const { return hwaccel; }

    static void initSoftwareCursor();
    static QScreenCursor* instance() { return qt_screencursor; }

protected:
    QImage cursor;

    QSize size;
    QPoint pos;
    QPoint hotspot;
    uint enable : 1;
    uint hwaccel : 1;
    uint supportsAlpha : 1;

private:
    friend class QProxyScreenCursor;
};

#endif // QT_NO_QWS_CURSOR

// A (used) chunk of offscreen memory

class QPoolEntry
{
public:
    unsigned int start;
    unsigned int end;
    int clientId;
};

class QScreen;
class QScreenPrivate;
class QPixmapDataFactory;

extern Q_GUI_EXPORT QScreen *qt_screen;
typedef void(*ClearCacheFunc)(QScreen *obj, int);

class Q_GUI_EXPORT QScreen {

public:
    enum ClassId { LinuxFBClass, TransformedClass, VNCClass, MultiClass,
                   VFbClass, DirectFBClass, SvgalibClass, ProxyClass,
                   GLClass, IntfbClass, CustomClass = 1024 };

    QScreen(int display_id, ClassId classId);
    explicit QScreen(int display_id);
    virtual ~QScreen();
    static QScreen* instance() { return qt_screen; }
    virtual bool initDevice() = 0;
    virtual bool connect(const QString &displaySpec) = 0;
    virtual void disconnect() = 0;
    virtual void shutdownDevice();
    virtual void setMode(int,int,int) = 0;
    virtual bool supportsDepth(int) const;

    virtual void save();
    virtual void restore();
    virtual void blank(bool on);

    virtual int pixmapOffsetAlignment() { return 64; }
    virtual int pixmapLinestepAlignment() { return 64; }
    virtual int sharedRamSize(void *) { return 0; }

    virtual bool onCard(const unsigned char *) const;
    virtual bool onCard(const unsigned char *, ulong& out_offset) const;

    enum PixelType { NormalPixel, BGRPixel };

    // sets a single color in the colormap
    virtual void set(unsigned int,unsigned int,unsigned int,unsigned int);
    // allocates a color
    virtual int alloc(unsigned int,unsigned int,unsigned int);

    int width() const { return w; }
    int height() const { return h; }
    int depth() const { return d; }
    virtual int pixmapDepth() const;
    PixelType pixelType() const { return pixeltype; }
    int linestep() const { return lstep; }
    int deviceWidth() const { return dw; }
    int deviceHeight() const { return dh; }
    uchar * base() const { return data; }
    // Ask for memory from card cache with alignment
    virtual uchar * cache(int) { return 0; }
    virtual void uncache(uchar *) {}

    QImage::Format pixelFormat() const;

    int screenSize() const { return size; }
    int totalSize() const { return mapsize; }

    QRgb * clut() { return screenclut; }
#ifdef QT_DEPRECATED
    QT_DEPRECATED int numCols() { return screencols; }
#endif
    int colorCount() { return screencols; }

    virtual QSize mapToDevice(const QSize &) const;
    virtual QSize mapFromDevice(const QSize &) const;
    virtual QPoint mapToDevice(const QPoint &, const QSize &) const;
    virtual QPoint mapFromDevice(const QPoint &, const QSize &) const;
    virtual QRect mapToDevice(const QRect &, const QSize &) const;
    virtual QRect mapFromDevice(const QRect &, const QSize &) const;
    virtual QImage mapToDevice(const QImage &) const;
    virtual QImage mapFromDevice(const QImage &) const;
    virtual QRegion mapToDevice(const QRegion &, const QSize &) const;
    virtual QRegion mapFromDevice(const QRegion &, const QSize &) const;
    virtual int transformOrientation() const;
    virtual bool isTransformed() const;
    virtual bool isInterlaced() const;

    virtual void setDirty(const QRect&);

    virtual int memoryNeeded(const QString&);

    virtual void haltUpdates();
    virtual void resumeUpdates();

    // composition manager methods
    virtual void exposeRegion(QRegion r, int changing);

    // these work directly on the screen
    virtual void blit(const QImage &img, const QPoint &topLeft, const QRegion &region);
    virtual void solidFill(const QColor &color, const QRegion &region);
    void blit(QWSWindow *bs, const QRegion &clip);

    virtual QWSWindowSurface* createSurface(QWidget *widget) const;
    virtual QWSWindowSurface* createSurface(const QString &key) const;

    virtual QList<QScreen*> subScreens() const { return QList<QScreen*>(); }
    virtual QRegion region() const { return QRect(offset(), QSize(w, h)); }
    int subScreenIndexAt(const QPoint &p) const;

    void setOffset(const QPoint &p);
    QPoint offset() const;

    int physicalWidth() const { return physWidth; }   // physical display size in mm
    int physicalHeight() const { return physHeight; } // physical display size in mm

    QPixmapDataFactory* pixmapDataFactory() const; // Deprecated, will be removed in 4.6
    QGraphicsSystem* graphicsSystem() const;

#ifdef QT_QWS_CLIENTBLIT
    bool supportsBlitInClients() const;
    void setSupportsBlitInClients(bool);
#endif

    ClassId classId() const;

protected:
    void setPixelFormat(QImage::Format format);
    void setPixmapDataFactory(QPixmapDataFactory *factory); // Deprecated, will be removed in 4.6
    void setGraphicsSystem(QGraphicsSystem* system);

    QRgb screenclut[256];
    int screencols;

    uchar * data;

    // Table of allocated lumps, kept in sorted highest-to-lowest order
    // The table itself is allocated at the bottom of offscreen memory
    // i.e. it's similar to having a stack (the table) and a heap
    // (the allocated blocks). Freed space is implicitly described
    // by the gaps between the allocated lumps (this saves entries and
    // means we don't need to worry about coalescing freed lumps)

    QPoolEntry * entries;
    int * entryp;
    unsigned int * lowest;

    int w;
    int lstep;
    int h;
    int d;
    PixelType pixeltype;
    bool grayscale;

    int dw;
    int dh;

    int size;               // Screen size
    int mapsize;       // Total mapped memory

    int displayId;

    int physWidth;
    int physHeight;

    friend class QWSServer;
    friend class QWSServerPrivate;
    static ClearCacheFunc clearCacheFunc;

private:
    void compose(int level, const QRegion &exposed, QRegion &blend,
                 QImage **blendbuffer, int changing_level);
    void paintBackground(const QRegion &);

    friend class QWSOnScreenSurface;
    static bool isWidgetPaintOnScreen(const QWidget *w);

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    void setFrameBufferLittleEndian(bool littleEndian);
    bool frameBufferLittleEndian() const;
    friend class QVNCScreen;
    friend class QLinuxFbScreen;
    friend class QVFbScreen;
    friend class QProxyScreen;
    friend class QIntfbScreen;
#endif
    friend void qt_solidFill_setup(QScreen*, const QColor&, const QRegion&);
    friend void qt_blit_setup(QScreen *screen, const QImage &image,
                              const QPoint &topLeft, const QRegion &region);
#ifdef QT_QWS_DEPTH_GENERIC
    friend void qt_set_generic_blit(QScreen *screen, int bpp,
                                    int len_red, int len_green, int len_blue,
                                    int len_alpha, int off_red, int off_green,
                                    int off_blue, int off_alpha);
#endif

    QScreenPrivate *d_ptr;
};

// This lives in loadable modules

#ifndef QT_LOADABLE_MODULES
extern "C" QScreen * qt_get_screen(int display_id, const char* spec);
#endif

// This is in main lib, loads the right module, calls qt_get_screen
// In non-loadable cases just aliases to qt_get_screen

const unsigned char * qt_probe_bus();

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSCREEN_QWS_H
