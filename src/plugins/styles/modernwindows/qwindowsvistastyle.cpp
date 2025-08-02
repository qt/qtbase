// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsvistastyle_p.h"
#include "qwindowsvistastyle_p_p.h"
#include "qwindowsvistaanimation_p.h"
#include <qoperatingsystemversion.h>
#include <qpainterstateguard.h>
#include <qscreen.h>
#include <qstylehints.h>
#include <qwindow.h>
#include <private/qstyleanimation_p.h>
#include <private/qstylehelper_p.h>
#include <qpa/qplatformnativeinterface.h>
#include <private/qapplication_p.h>
#include <private/qsystemlibrary_p.h>
#include <private/qwindowsthemecache_p.h>

#include "qdrawutil.h" // for now
#include <qbackingstore.h>


QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static constexpr int windowsItemFrame        =  2; // menu item frame width
static constexpr int windowsItemHMargin      =  3; // menu item hor text margin
static constexpr int windowsItemVMargin      =  4; // menu item ver text margin
static constexpr int windowsArrowHMargin     =  6; // arrow horizontal margin
static constexpr int windowsRightBorder      = 15; // right border on windows

#ifndef TMT_CONTENTMARGINS
#  define TMT_CONTENTMARGINS 3602
#endif
#ifndef TMT_SIZINGMARGINS
#  define TMT_SIZINGMARGINS 3601
#endif
#ifndef LISS_NORMAL
#  define LISS_NORMAL 1
#  define LISS_HOT 2
#  define LISS_SELECTED 3
#  define LISS_DISABLED 4
#  define LISS_SELECTEDNOTFOCUS 5
#  define LISS_HOTSELECTED 6
#endif
#ifndef BP_COMMANDLINK
#  define BP_COMMANDLINK 6
#  define BP_COMMANDLINKGLYPH 7
#  define CMDLGS_NORMAL 1
#  define CMDLGS_HOT 2
#  define CMDLGS_PRESSED 3
#  define CMDLGS_DISABLED 4
#endif

// QWindowsVistaStylePrivate -------------------------------------------------------------------------
// Static initializations
QVarLengthFlatMap<const QScreen *, HWND, 4> QWindowsVistaStylePrivate::m_vistaTreeViewHelpers;
bool QWindowsVistaStylePrivate::useVistaTheme = false;
Q_CONSTINIT QBasicAtomicInt QWindowsVistaStylePrivate::ref = Q_BASIC_ATOMIC_INITIALIZER(-1); // -1 based refcounting

static void qt_add_rect(HRGN &winRegion, QRect r)
{
    HRGN rgn = CreateRectRgn(r.left(), r.top(), r.x() + r.width(), r.y() + r.height());
    if (rgn) {
        HRGN dest = CreateRectRgn(0,0,0,0);
        int result = CombineRgn(dest, winRegion, rgn, RGN_OR);
        if (result) {
            DeleteObject(winRegion);
            winRegion = dest;
        }
        DeleteObject(rgn);
    }
}

static HRGN qt_hrgn_from_qregion(const QRegion &region)
{
    HRGN hRegion = CreateRectRgn(0,0,0,0);
    if (region.rectCount() == 1) {
        qt_add_rect(hRegion, region.boundingRect());
        return hRegion;
    }
    for (const QRect &rect : region)
        qt_add_rect(hRegion, rect);
    return hRegion;
}

static inline Qt::Orientation progressBarOrientation(const QStyleOption *option = nullptr)
{
    if (const auto *pb = qstyleoption_cast<const QStyleOptionProgressBar *>(option))
        return pb->state & QStyle::State_Horizontal ? Qt::Horizontal : Qt::Vertical;
    return Qt::Horizontal;
}

/* In order to obtain the correct VistaTreeViewTheme (arrows for PE_IndicatorBranch),
 * we need to set the windows "explorer" theme explicitly on a native
 * window and open the "TREEVIEW" theme handle passing its window handle
 * in order to get Vista-style item view themes (particularly drawBackground()
 * for selected items needs this).
 * We invoke a service of the native Windows interface to create
 * a non-visible window handle, open the theme on it and insert it into
 * the cache so that it is found by QWindowsThemeData::handle() first.
 */
static inline HWND createTreeViewHelperWindow(const QScreen *screen)
{
    using QWindowsApplication = QNativeInterface::Private::QWindowsApplication;

    HWND result = nullptr;
    if (auto nativeWindowsApp = dynamic_cast<QWindowsApplication *>(QGuiApplicationPrivate::platformIntegration()))
        result = nativeWindowsApp->createMessageWindow(QStringLiteral(u"QTreeViewThemeHelperWindowClass"),
                                                       QStringLiteral(u"QTreeViewThemeHelperWindow"));
    const auto topLeft = screen->geometry().topLeft();
    // make it a top-level window and move it the the correct screen to paint with the correct dpr later on
    SetParent(result, NULL);
    MoveWindow(result, topLeft.x(), topLeft.y(), 10, 10, FALSE);
    return result;
}

enum TransformType { SimpleTransform, HighDpiScalingTransform, ComplexTransform };

static inline TransformType transformType(const QTransform &transform, qreal devicePixelRatio)
{
    if (transform.type() <= QTransform::TxTranslate)
        return SimpleTransform;
    if (transform.type() > QTransform::TxScale)
        return ComplexTransform;
    return qFuzzyCompare(transform.m11(), devicePixelRatio)
            && qFuzzyCompare(transform.m22(), devicePixelRatio)
            ? HighDpiScalingTransform : ComplexTransform;
}

// QTBUG-60571: Exclude known fully opaque theme parts which produce values
// invalid in ARGB32_Premultiplied (for example, 0x00ffffff).
static inline bool isFullyOpaque(const QWindowsThemeData &themeData)
{
    return themeData.theme == QWindowsVistaStylePrivate::TaskDialogTheme && themeData.partId == TDLG_PRIMARYPANEL;
}

static inline QRectF scaleRect(const QRectF &r, qreal factor)
{
    return r.isValid() && factor > 1
            ? QRectF(r.topLeft() * factor, r.size() * factor) : r;
}

static QRegion scaleRegion(const QRegion &region, qreal factor)
{
    if (region.isEmpty() || qFuzzyCompare(factor, qreal(1)))
        return region;
    QRegion result;
    for (const QRect &rect : region)
        result += QRectF(QPointF(rect.topLeft()) * factor, QSizeF(rect.size() * factor)).toRect();
    return result;
}


/* \internal
    Checks if the theme engine can/should be used, or if we should fall back
    to Windows style. For Windows 10, this will still return false for the
    High Contrast themes.
*/
bool QWindowsVistaStylePrivate::useVista(bool update)
{
    if (update)
        useVistaTheme = IsThemeActive() && (IsAppThemed() || !QCoreApplication::instance());
    return useVistaTheme;
}

/* \internal
    Handles refcounting, and queries the theme engine for usage.
*/
void QWindowsVistaStylePrivate::init(bool force)
{
    if (ref.ref() && !force)
        return;
    if (!force) // -1 based atomic refcounting
        ref.ref();

    useVista(true);
}

/* \internal
    Cleans up all static data.
*/
void QWindowsVistaStylePrivate::cleanup(bool force)
{
    if (bufferBitmap) {
        if (bufferDC && nullBitmap)
            SelectObject(bufferDC, nullBitmap);
        DeleteObject(bufferBitmap);
        bufferBitmap = nullptr;
    }

    if (bufferDC)
        DeleteDC(bufferDC);
    bufferDC = nullptr;

    if (ref.deref() && !force)
        return;
    if (!force)  // -1 based atomic refcounting
        ref.deref();

    useVistaTheme = false;
    cleanupHandleMap();
}

bool QWindowsVistaStylePrivate::transitionsEnabled() const
{
    Q_Q(const QWindowsVistaStyle);
    if (q->property("_q_no_animation").toBool())
        return false;
    if (QApplication::desktopSettingsAware()) {
        BOOL animEnabled = false;
        if (SystemParametersInfo(SPI_GETCLIENTAREAANIMATION, 0, &animEnabled, 0)) {
            if (animEnabled)
                return true;
        }
    }
    return false;
}

int QWindowsVistaStylePrivate::pixelMetricFromSystemDp(QStyle::PixelMetric pm, const QStyleOption *option, const QWidget *widget)
{
    switch (pm) {
    case QStyle::PM_IndicatorWidth:
        return QWindowsThemeData::themeSize(widget, nullptr, QWindowsVistaStylePrivate::ButtonTheme, BP_CHECKBOX, CBS_UNCHECKEDNORMAL).width();
    case QStyle::PM_IndicatorHeight:
        return QWindowsThemeData::themeSize(widget, nullptr, QWindowsVistaStylePrivate::ButtonTheme, BP_CHECKBOX, CBS_UNCHECKEDNORMAL).height();
    case QStyle::PM_ExclusiveIndicatorWidth:
        return QWindowsThemeData::themeSize(widget, nullptr, QWindowsVistaStylePrivate::ButtonTheme, BP_RADIOBUTTON, RBS_UNCHECKEDNORMAL).width();
    case QStyle::PM_ExclusiveIndicatorHeight:
        return QWindowsThemeData::themeSize(widget, nullptr, QWindowsVistaStylePrivate::ButtonTheme, BP_RADIOBUTTON, RBS_UNCHECKEDNORMAL).height();
    case QStyle::PM_ProgressBarChunkWidth:
        return progressBarOrientation(option) == Qt::Horizontal
                ? QWindowsThemeData::themeSize(widget, nullptr, QWindowsVistaStylePrivate::ProgressTheme, PP_CHUNK).width()
                : QWindowsThemeData::themeSize(widget, nullptr, QWindowsVistaStylePrivate::ProgressTheme, PP_CHUNKVERT).height();
    case QStyle::PM_SliderThickness:
        return QWindowsThemeData::themeSize(widget, nullptr, QWindowsVistaStylePrivate::TrackBarTheme, TKP_THUMB).height();
    case QStyle::PM_TitleBarHeight:
        return QWindowsStylePrivate::pixelMetricFromSystemDp(QStyle::PM_TitleBarHeight, option, widget);
    case QStyle::PM_MdiSubWindowFrameWidth:
        return QWindowsThemeData::themeSize(widget, nullptr, QWindowsVistaStylePrivate::WindowTheme, WP_FRAMELEFT, FS_ACTIVE).width();
    case QStyle::PM_DockWidgetFrameWidth:
        return QWindowsThemeData::themeSize(widget, nullptr, QWindowsVistaStylePrivate::WindowTheme, WP_SMALLFRAMERIGHT, FS_ACTIVE).width();
    default:
        break;
    }
    return QWindowsVistaStylePrivate::InvalidMetric;
}

int QWindowsVistaStylePrivate::fixedPixelMetric(QStyle::PixelMetric pm)
{
    switch (pm) {
    case QStyle::PM_DockWidgetTitleBarButtonMargin:
        return 5;
    case QStyle::PM_ScrollBarSliderMin:
        return 18;
    case QStyle::PM_MenuHMargin:
    case QStyle::PM_MenuVMargin:
        return 0;
    case QStyle::PM_MenuPanelWidth:
        return 3;
    default:
        break;
    }

    return QWindowsVistaStylePrivate::InvalidMetric;
}

bool QWindowsVistaStylePrivate::initVistaTreeViewTheming(const QScreen *screen)
{
    if (m_vistaTreeViewHelpers.contains(screen))
        return true;
    HWND helper = createTreeViewHelperWindow(screen);
    if (FAILED(SetWindowTheme(helper, L"explorer", nullptr)))
    {
        qErrnoWarning("SetWindowTheme() failed.");
        cleanupVistaTreeViewTheming();
        return false;
    }
    m_vistaTreeViewHelpers.insert(screen, helper);
    return true;
}

void QWindowsVistaStylePrivate::cleanupVistaTreeViewTheming()
{
    for (auto it = m_vistaTreeViewHelpers.begin(); it != m_vistaTreeViewHelpers.end(); ++it)
        DestroyWindow(it.value());
    m_vistaTreeViewHelpers.clear();
}

/* \internal
    Closes all open theme data handles to ensure that we don't leak
    resources, and that we don't refer to old handles when for
    example the user changes the theme style.
*/
void QWindowsVistaStylePrivate::cleanupHandleMap()
{
    QWindowsThemeCache::clearAllThemeCaches();
    QWindowsVistaStylePrivate::cleanupVistaTreeViewTheming();
}

HTHEME QWindowsVistaStylePrivate::createTheme(int theme, const QWidget *widget)
{
    const QScreen *screen = widget ? widget->screen() : qApp->primaryScreen();
    HWND hwnd = QWindowsVistaStylePrivate::winId(widget);
    if (theme == VistaTreeViewTheme && QWindowsVistaStylePrivate::initVistaTreeViewTheming(screen))
        hwnd = QWindowsVistaStylePrivate::m_vistaTreeViewHelpers.value(screen);
    return QWindowsThemeCache::createTheme(theme, hwnd);
}

QBackingStore *QWindowsVistaStylePrivate::backingStoreForWidget(const QWidget *widget)
{
    if (QBackingStore *backingStore = widget->backingStore())
        return backingStore;
    if (const QWidget *topLevel = widget->nativeParentWidget())
        if (QBackingStore *topLevelBackingStore = topLevel->backingStore())
            return topLevelBackingStore;
    return nullptr;
}

HDC QWindowsVistaStylePrivate::hdcForWidgetBackingStore(const QWidget *widget)
{
    if (QBackingStore *backingStore = backingStoreForWidget(widget)) {
        QPlatformNativeInterface *nativeInterface = QGuiApplication::platformNativeInterface();
        if (nativeInterface)
            return static_cast<HDC>(nativeInterface->nativeResourceForBackingStore(QByteArrayLiteral("getDC"), backingStore));
    }
    return nullptr;
}

QString QWindowsVistaStylePrivate::themeName(int theme)
{
    return QWindowsThemeCache::themeName(theme);
}

bool QWindowsVistaStylePrivate::isItemViewDelegateLineEdit(const QWidget *widget)
{
    if (!widget)
        return false;
    const QWidget *parent1 = widget->parentWidget();
    // Exclude dialogs or other toplevels parented on item views.
    if (!parent1 || parent1->isWindow())
        return false;
    const QWidget *parent2 = parent1->parentWidget();
    return parent2 && widget->inherits("QLineEdit")
            && parent2->inherits("QAbstractItemView");
}

// Returns whether base color is set for this widget
bool QWindowsVistaStylePrivate::isLineEditBaseColorSet(const QStyleOption *option, const QWidget *widget)
{
    uint resolveMask = option->palette.resolveMask();
    if (widget) {
        // Since spin box includes a line edit we need to resolve the palette mask also from
        // the parent, as while the color is always correct on the palette supplied by panel,
        // the mask can still be empty. If either mask specifies custom base color, use that.
#if QT_CONFIG(spinbox)
        if (const QAbstractSpinBox *spinbox = qobject_cast<QAbstractSpinBox*>(widget->parentWidget()))
            resolveMask |= spinbox->palette().resolveMask();
#endif // QT_CONFIG(spinbox)
    }
    return (resolveMask & (1 << QPalette::Base)) != 0;
}

/*! \internal
    This function will always return a valid window handle, and might
    create a limbo widget to do so.
    We often need a window handle to for example open theme data, so
    this function ensures that we get one.
*/
HWND QWindowsVistaStylePrivate::winId(const QWidget *widget)
{
    if (widget) {
        if (const HWND hwnd = QApplicationPrivate::getHWNDForWidget(const_cast<QWidget *>(widget)))
            return hwnd;
    }

    // Find top level with native window (there might be dialogs that do not have one).
    const auto allWindows = QGuiApplication::allWindows();
    for (const QWindow *window : allWindows) {
        if (window->isTopLevel() && window->type() != Qt::Desktop && window->handle() != nullptr)
            return reinterpret_cast<HWND>(window->winId());
    }

    return GetDesktopWindow();
}

/*! \internal
    Returns a native buffer (DIB section) of at least the size of
    ( \a x , \a y ). The buffer has a 32 bit depth, to not lose
    the alpha values on proper alpha-pixmaps.
*/
HBITMAP QWindowsVistaStylePrivate::buffer(int w, int h)
{
    // If we already have a HBITMAP which is of adequate size, just return that
    if (bufferBitmap) {
        if (bufferW >= w && bufferH >= h)
            return bufferBitmap;
        // Not big enough, discard the old one
        if (bufferDC && nullBitmap)
            SelectObject(bufferDC, nullBitmap);
        DeleteObject(bufferBitmap);
        bufferBitmap = nullptr;
    }

    w = qMax(bufferW, w);
    h = qMax(bufferH, h);

    if (!bufferDC) {
        HDC displayDC = GetDC(nullptr);
        bufferDC = CreateCompatibleDC(displayDC);
        ReleaseDC(nullptr, displayDC);
    }

    // Define the header
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = w;
    bmi.bmiHeader.biHeight      = -h;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    // Create the pixmap
    bufferPixels = nullptr;
    bufferBitmap = CreateDIBSection(bufferDC, &bmi, DIB_RGB_COLORS, reinterpret_cast<void **>(&bufferPixels), nullptr, 0);
    GdiFlush();
    nullBitmap = static_cast<HBITMAP>(SelectObject(bufferDC, bufferBitmap));

    if (Q_UNLIKELY(!bufferBitmap)) {
        qErrnoWarning("QWindowsVistaStylePrivate::buffer(%dx%d), CreateDIBSection() failed.", w, h);
        bufferW = 0;
        bufferH = 0;
        return nullptr;
    }
    if (Q_UNLIKELY(!bufferPixels)) {
        qErrnoWarning("QWindowsVistaStylePrivate::buffer(%dx%d), CreateDIBSection() did not allocate pixel data.", w, h);
        bufferW = 0;
        bufferH = 0;
        return nullptr;
    }
    bufferW = w;
    bufferH = h;
#ifdef DEBUG_XP_STYLE
    qDebug("Creating new dib section (%d, %d)", w, h);
#endif
    return bufferBitmap;
}

/*! \internal
    Returns \c true if the part contains any transparency at all. This does
    not indicate what kind of transparency we're dealing with. It can be
        - Alpha transparency
        - Masked transparency
*/
bool QWindowsVistaStylePrivate::isTransparent(QWindowsThemeData &themeData)
{
    return IsThemeBackgroundPartiallyTransparent(themeData.handle(), themeData.partId,
                                                 themeData.stateId);
}


/*! \internal
    Returns a QRegion of the region of the part
*/
QRegion QWindowsVistaStylePrivate::region(QWindowsThemeData &themeData)
{
    HRGN hRgn = nullptr;
    const qreal factor = QWindowsStylePrivate::nativeMetricScaleFactor(themeData.widget);
    RECT rect = themeData.toRECT(QRect(themeData.rect.topLeft() / factor, themeData.rect.size() / factor));
    if (!SUCCEEDED(GetThemeBackgroundRegion(themeData.handle(), bufferHDC(), themeData.partId,
                                            themeData.stateId, &rect, &hRgn))) {
        return QRegion();
    }

    HRGN dest = CreateRectRgn(0, 0, 0, 0);
    const bool success = CombineRgn(dest, hRgn, nullptr, RGN_COPY) != ERROR;

    QRegion region;

    if (success) {
        QVarLengthArray<char> buf(256);
        RGNDATA *rd = reinterpret_cast<RGNDATA *>(buf.data());
        if (GetRegionData(dest, buf.size(), rd) == 0) {
            const auto numBytes = GetRegionData(dest, 0, nullptr);
            if (numBytes > 0) {
                buf.resize(numBytes);
                rd = reinterpret_cast<RGNDATA *>(buf.data());
                if (GetRegionData(dest, numBytes, rd) == 0)
                    rd = nullptr;
            } else {
                rd = nullptr;
            }
        }
        if (rd) {
            RECT *r = reinterpret_cast<RECT *>(rd->Buffer);
            for (uint i = 0; i < rd->rdh.nCount; ++i) {
                QRect rect;
                rect.setCoords(int(r->left * factor), int(r->top * factor),
                               int((r->right - 1) * factor), int((r->bottom - 1) * factor));
                ++r;
                region |= rect;
            }
        }
    }

    DeleteObject(hRgn);
    DeleteObject(dest);

    return region;
}

/*! \internal
    Returns \c true if the native doublebuffer contains pixels with
    varying alpha value.
*/
bool QWindowsVistaStylePrivate::hasAlphaChannel(const QRect &rect)
{
    const int startX = rect.left();
    const int startY = rect.top();
    const int w = rect.width();
    const int h = rect.height();

    int firstAlpha = -1;
    for (int y = startY; y < h/2; ++y) {
        auto buffer = reinterpret_cast<const DWORD *>(bufferPixels) + (y * bufferW);
        for (int x = startX; x < w; ++x, ++buffer) {
            int alpha = (*buffer) >> 24;
            if (firstAlpha == -1)
                firstAlpha = alpha;
            else if (alpha != firstAlpha)
                return true;
        }
    }
    return false;
}

/*! \internal
    When the theme engine paints both a true alpha pixmap and a glyph
    into our buffer, the glyph might not contain a proper alpha value.
    The rule of thumb for premultiplied pixmaps is that the color
    values of a pixel can never be higher than the alpha values, so
    we use this to our advantage here, and fix all instances where
    this occurs.
*/
bool QWindowsVistaStylePrivate::fixAlphaChannel(const QRect &rect)
{
    const int startX = rect.left();
    const int startY = rect.top();
    const int w = rect.width();
    const int h = rect.height();
    bool hasFixedAlphaValue = false;

    for (int y = startY; y < h; ++y) {
        auto buffer = reinterpret_cast<DWORD *>(bufferPixels) + (y * bufferW);
        for (int x = startX; x < w; ++x, ++buffer) {
            uint pixel = *buffer;
            int alpha = qAlpha(pixel);
            if (qRed(pixel) > alpha || qGreen(pixel) > alpha || qBlue(pixel) > alpha) {
                *buffer |= 0xff000000;
                hasFixedAlphaValue = true;
            }
        }
    }
    return hasFixedAlphaValue;
}

/*! \internal
    Swaps the alpha values on certain pixels:
        0xFF?????? -> 0x00??????
        0x00?????? -> 0xFF??????
    Used to determine the mask of a non-alpha transparent pixmap in
    the native doublebuffer, and swap the alphas so we may paint
    the image as a Premultiplied QImage with drawImage(), and obtain
    the mask transparency.
*/
bool QWindowsVistaStylePrivate::swapAlphaChannel(const QRect &rect, bool allPixels)
{
    const int startX = rect.left();
    const int startY = rect.top();
    const int w = rect.width();
    const int h = rect.height();
    bool valueChange = false;

    // Flip the alphas, so that 255-alpha pixels are 0, and 0-alpha are 255.
    for (int y = startY; y < h; ++y) {
        auto buffer = reinterpret_cast<DWORD *>(bufferPixels) + (y * bufferW);
        for (int x = startX; x < w; ++x, ++buffer) {
            if (allPixels) {
                *buffer |= 0xFF000000;
                continue;
            }
            unsigned int alphaValue = (*buffer) & 0xFF000000;
            if (alphaValue == 0xFF000000) {
                *buffer = 0;
                valueChange = true;
            } else if (alphaValue == 0) {
                *buffer |= 0xFF000000;
                valueChange = true;
            }
        }
    }
    return valueChange;
}

/*! \internal
    Main theme drawing function.
    Determines the correct lowlevel drawing method depending on several
    factors.
        Use drawBackgroundThruNativeBuffer() if:
            - Painter does not have an HDC
            - Theme part is flipped (mirrored horizontally)
        else use drawBackgroundDirectly().
    \note drawBackgroundThruNativeBuffer() can return false for large
    sizes due to buffer()/CreateDIBSection() failing.
*/
bool QWindowsVistaStylePrivate::drawBackground(QWindowsThemeData &themeData, qreal correctionFactor)
{
    if (themeData.rect.isEmpty())
        return true;

    QPainter *painter = themeData.painter;
    Q_ASSERT_X(painter != nullptr, "QWindowsVistaStylePrivate::drawBackground()", "Trying to draw a theme part without a painter");
    if (!painter || !painter->isActive())
        return false;

    painter->save();

    // Access paintDevice via engine since the painter may
    // return the clip device which can still be a widget device in case of grabWidget().

    bool translucentToplevel = false;
    const QPaintDevice *paintDevice = painter->device();
    const qreal aditionalDevicePixelRatio = themeData.widget ? themeData.widget->devicePixelRatio() : qreal(1);
    if (paintDevice->devType() == QInternal::Widget) {
        const QWidget *window = static_cast<const QWidget *>(paintDevice)->window();
        translucentToplevel = window->testAttribute(Qt::WA_TranslucentBackground);
    }

    const TransformType tt = transformType(painter->deviceTransform(), aditionalDevicePixelRatio);

    bool canDrawDirectly = false;
    if (themeData.widget && painter->opacity() == 1.0 && !themeData.rotate
            && !isFullyOpaque(themeData)
            && tt != ComplexTransform && !themeData.mirrorVertically && !themeData.invertPixels
            && !translucentToplevel) {
        // Draw on backing store DC only for real widgets or backing store images.
        const QPaintDevice *enginePaintDevice = painter->paintEngine()->paintDevice();
        switch (enginePaintDevice->devType()) {
        case QInternal::Widget:
            canDrawDirectly = true;
            break;
        case QInternal::Image:
            // Ensure the backing store has received as resize and is initialized.
            if (QBackingStore *bs = backingStoreForWidget(themeData.widget)) {
                if (bs->size().isValid() && bs->paintDevice() == enginePaintDevice)
                    canDrawDirectly = true;
            }
            break;
        }
    }

    const HDC dc = canDrawDirectly ? hdcForWidgetBackingStore(themeData.widget) : nullptr;
    const bool result = dc && qFuzzyCompare(correctionFactor, qreal(1))
            ? drawBackgroundDirectly(dc, themeData, aditionalDevicePixelRatio)
            : drawBackgroundThruNativeBuffer(themeData, aditionalDevicePixelRatio, correctionFactor);
    painter->restore();
    return result;
}

/*! \internal
    This function draws the theme parts directly to the paintengines HDC.
    Do not use this if you need to perform other transformations on the
    resulting data.
*/
bool QWindowsVistaStylePrivate::drawBackgroundDirectly(HDC dc, QWindowsThemeData &themeData, qreal additionalDevicePixelRatio)
{
    QPainter *painter = themeData.painter;

    const auto &deviceTransform = painter->deviceTransform();
    const QPointF redirectionDelta(deviceTransform.dx(), deviceTransform.dy());
    const QRect area = scaleRect(QRectF(themeData.rect), additionalDevicePixelRatio).translated(redirectionDelta).toRect();

    QRegion sysRgn = painter->paintEngine()->systemClip();
    if (sysRgn.isEmpty())
        sysRgn = area;
    else
        sysRgn &= area;
    if (painter->hasClipping())
        sysRgn &= scaleRegion(painter->clipRegion(), additionalDevicePixelRatio).translated(redirectionDelta.toPoint());
    HRGN hrgn = qt_hrgn_from_qregion(sysRgn);
    SelectClipRgn(dc, hrgn);

#ifdef DEBUG_XP_STYLE
    printf("---[ DIRECT PAINTING ]------------------> Name(%-10s) Part(%d) State(%d)\n",
           qPrintable(themeData.name), themeData.partId, themeData.stateId);
    showProperties(themeData);
#endif

    RECT drawRECT = themeData.toRECT(area);
    DTBGOPTS drawOptions;
    memset(&drawOptions, 0, sizeof(drawOptions));
    drawOptions.dwSize = sizeof(drawOptions);
    drawOptions.rcClip = themeData.toRECT(sysRgn.boundingRect());
    drawOptions.dwFlags = DTBG_CLIPRECT
            | (themeData.noBorder ? DTBG_OMITBORDER : 0)
            | (themeData.noContent ? DTBG_OMITCONTENT : 0)
            | (themeData.mirrorHorizontally ? DTBG_MIRRORDC : 0);

    const HRESULT result = DrawThemeBackgroundEx(themeData.handle(), dc, themeData.partId, themeData.stateId, &(drawRECT), &drawOptions);
    SelectClipRgn(dc, nullptr);
    DeleteObject(hrgn);
    return SUCCEEDED(result);
}

/*! \internal
    This function uses a secondary Native doublebuffer for painting parts.
    It should only be used when the painteengine doesn't provide a proper
    HDC for direct painting (e.g. when doing a grabWidget(), painting to
    other pixmaps etc), or when special transformations are needed (e.g.
    flips (horizontal mirroring only, vertical are handled by the theme
    engine).

    \a correctionFactor is an additional factor used to scale up controls
    that are too small on High DPI screens, as has been observed for
    WP_MDICLOSEBUTTON, WP_MDIRESTOREBUTTON, WP_MDIMINBUTTON (QTBUG-75927).
*/
bool QWindowsVistaStylePrivate::drawBackgroundThruNativeBuffer(QWindowsThemeData &themeData,
                                                               qreal additionalDevicePixelRatio,
                                                               qreal correctionFactor)
{
    QPainter *painter = themeData.painter;
    QRectF rectF = scaleRect(QRectF(themeData.rect), additionalDevicePixelRatio);

    if ((themeData.rotate + 90) % 180 == 0) { // Catch 90,270,etc.. degree flips.
        rectF = QRectF(0, 0, rectF.height(), rectF.width());
    }
    rectF.moveTo(0, 0);

    const bool hasCorrectionFactor = !qFuzzyCompare(correctionFactor, qreal(1));
    QRect rect = rectF.toRect();
    const QRect drawRect = hasCorrectionFactor
            ? QRectF(rectF.topLeft() / correctionFactor, rectF.size() / correctionFactor).toRect()
            : rect;
    int partId = themeData.partId;
    int stateId = themeData.stateId;
    int w = rect.width();
    int h = rect.height();

    // Values initialized later, either from cached values, or from function calls
    AlphaChannelType alphaType = UnknownAlpha;
    bool stateHasData = true; // We assume so;
    bool hasAlpha = false;
    bool partIsTransparent;
    bool potentialInvalidAlpha;

    QString pixmapCacheKey = QStringLiteral(u"$qt_xp_");
    pixmapCacheKey.append(themeName(themeData.theme));
    pixmapCacheKey.append(QLatin1Char('p'));
    pixmapCacheKey.append(QString::number(partId));
    pixmapCacheKey.append(QLatin1Char('s'));
    pixmapCacheKey.append(QString::number(stateId));
    pixmapCacheKey.append(QLatin1Char('s'));
    pixmapCacheKey.append(themeData.noBorder ? QLatin1Char('0') : QLatin1Char('1'));
    pixmapCacheKey.append(QLatin1Char('b'));
    pixmapCacheKey.append(themeData.noContent ? QLatin1Char('0') : QLatin1Char('1'));
    pixmapCacheKey.append(QString::number(w));
    pixmapCacheKey.append(QLatin1Char('w'));
    pixmapCacheKey.append(QString::number(h));
    pixmapCacheKey.append(QLatin1Char('h'));
    pixmapCacheKey.append(QString::number(additionalDevicePixelRatio));
    pixmapCacheKey.append(QLatin1Char('d'));
    if (hasCorrectionFactor) {
        pixmapCacheKey.append(QLatin1Char('c'));
        pixmapCacheKey.append(QString::number(correctionFactor));
    }

    QPixmap cachedPixmap;
    ThemeMapKey key(themeData);
    ThemeMapData data = alphaCache.value(key);

    bool haveCachedPixmap = false;
    bool isCached = data.dataValid;
    if (isCached) {
        partIsTransparent = data.partIsTransparent;
        hasAlpha = data.hasAlphaChannel;
        alphaType = data.alphaType;
        potentialInvalidAlpha = data.hadInvalidAlpha;

        haveCachedPixmap = QPixmapCache::find(pixmapCacheKey, &cachedPixmap);

#ifdef DEBUG_XP_STYLE
        char buf[25];
        ::sprintf(buf, "+ Pixmap(%3d, %3d) ]", w, h);
        printf("---[ CACHED %s--------> Name(%-10s) Part(%d) State(%d)\n",
               haveCachedPixmap ? buf : "]-------------------",
               qPrintable(themeData.name), themeData.partId, themeData.stateId);
#endif
    } else {
        // Not cached, so get values from Theme Engine
        BOOL tmt_borderonly = false;
        COLORREF tmt_transparentcolor = 0x0;
        PROPERTYORIGIN proporigin = PO_NOTFOUND;
        GetThemeBool(themeData.handle(), themeData.partId, themeData.stateId, TMT_BORDERONLY, &tmt_borderonly);
        GetThemeColor(themeData.handle(), themeData.partId, themeData.stateId, TMT_TRANSPARENTCOLOR, &tmt_transparentcolor);
        GetThemePropertyOrigin(themeData.handle(), themeData.partId, themeData.stateId, TMT_CAPTIONMARGINS, &proporigin);

        partIsTransparent = isTransparent(themeData);

        potentialInvalidAlpha = false;
        GetThemePropertyOrigin(themeData.handle(), themeData.partId, themeData.stateId, TMT_GLYPHTYPE, &proporigin);
        if (proporigin == PO_PART || proporigin == PO_STATE) {
            int tmt_glyphtype = GT_NONE;
            GetThemeEnumValue(themeData.handle(), themeData.partId, themeData.stateId, TMT_GLYPHTYPE, &tmt_glyphtype);
            potentialInvalidAlpha = partIsTransparent && tmt_glyphtype == GT_IMAGEGLYPH;
        }

#ifdef DEBUG_XP_STYLE
        printf("---[ NOT CACHED ]-----------------------> Name(%-10s) Part(%d) State(%d)\n",
               qPrintable(themeData.name), themeData.partId, themeData.stateId);
        printf("-->partIsTransparen      = %d\n", partIsTransparent);
        printf("-->potentialInvalidAlpha = %d\n", potentialInvalidAlpha);
        showProperties(themeData);
#endif
    }
    bool wasAlphaSwapped = false;
    bool wasAlphaFixed = false;

    // OLD PSDK Workaround ------------------------------------------------------------------------
    // See if we need extra clipping for the older PSDK, which does
    // not have a DrawThemeBackgroundEx function for DTGB_OMITBORDER
    // and DTGB_OMITCONTENT
    bool addBorderContentClipping = false;
    QRegion extraClip;
    QRect area = drawRect;
    if (themeData.noBorder || themeData.noContent) {
        extraClip = area;
        // We are running on a system where the uxtheme.dll does not have
        // the DrawThemeBackgroundEx function, so we need to clip away
        // borders or contents manually.

        int borderSize = 0;
        PROPERTYORIGIN origin = PO_NOTFOUND;
        GetThemePropertyOrigin(themeData.handle(), themeData.partId, themeData.stateId, TMT_BORDERSIZE, &origin);
        GetThemeInt(themeData.handle(), themeData.partId, themeData.stateId, TMT_BORDERSIZE, &borderSize);
        borderSize *= additionalDevicePixelRatio;

        // Clip away border region
        if ((origin == PO_CLASS || origin == PO_PART || origin == PO_STATE) && borderSize > 0) {
            if (themeData.noBorder) {
                extraClip &= area;
                area = area.adjusted(-borderSize, -borderSize, borderSize, borderSize);
            }

            // Clip away content region
            if (themeData.noContent) {
                QRegion content = area.adjusted(borderSize, borderSize, -borderSize, -borderSize);
                extraClip ^= content;
            }
        }
        addBorderContentClipping = (themeData.noBorder | themeData.noContent);
    }

    QImage img;
    if (!haveCachedPixmap) { // If the pixmap is not cached, generate it! -------------------------
        if (!buffer(drawRect.width(), drawRect.height())) // Ensure a buffer of at least (w, h) in size
            return false;
        HDC dc = bufferHDC();

        // Clear the buffer
        if (alphaType != NoAlpha) {
            // Consider have separate "memset" function for small chunks for more speedup
            memset(bufferPixels, 0x00, bufferW * drawRect.height() * 4);
        }

        // Difference between area and rect
        int dx = area.x() - drawRect.x();
        int dy = area.y() - drawRect.y();

        // Adjust so painting rect starts from Origo
        rect.moveTo(0,0);
        area.moveTo(dx,dy);
        DTBGOPTS drawOptions;
        drawOptions.dwSize = sizeof(drawOptions);
        drawOptions.rcClip = themeData.toRECT(rect);
        drawOptions.dwFlags = DTBG_CLIPRECT
                | (themeData.noBorder ? DTBG_OMITBORDER : 0)
                | (themeData.noContent ? DTBG_OMITCONTENT : 0);

        // Drawing the part into the backing store
        RECT wRect(themeData.toRECT(area));
        DrawThemeBackgroundEx(themeData.handle(), dc, themeData.partId, themeData.stateId, &wRect, &drawOptions);

        // If not cached, analyze the buffer data to figure
        // out alpha type, and if it contains data
        if (!isCached) {
            // SHORTCUT: If the part's state has no data, cache it for NOOP later
            if (!stateHasData) {
                memset(static_cast<void *>(&data), 0, sizeof(data));
                data.dataValid = true;
                alphaCache.insert(key, data);
                return true;
            }
            hasAlpha = hasAlphaChannel(rect);
            if (!hasAlpha && partIsTransparent)
                potentialInvalidAlpha = true;
#if defined(DEBUG_XP_STYLE) && 1
            dumpNativeDIB(drawRect.width(), drawRect.height());
#endif
        }

        // Fix alpha values, if needed
        if (potentialInvalidAlpha)
            wasAlphaFixed = fixAlphaChannel(drawRect);

        QImage::Format format;
        if ((partIsTransparent && !wasAlphaSwapped) || (!partIsTransparent && hasAlpha)) {
            format = QImage::Format_ARGB32_Premultiplied;
            alphaType = RealAlpha;
        } else if (wasAlphaSwapped) {
            format = QImage::Format_ARGB32_Premultiplied;
            alphaType = MaskAlpha;
        } else {
            format = QImage::Format_RGB32;
            // The image data we got from the theme engine does not have any transparency,
            // thus the alpha channel is set to 0.
            // However, Format_RGB32 requires the alpha part to be set to 0xff, thus
            // we must flip it from 0x00 to 0xff
            swapAlphaChannel(rect, true);
            alphaType = NoAlpha;
        }
#if defined(DEBUG_XP_STYLE) && 1
        printf("Image format is: %s\n", alphaType == RealAlpha ? "Real Alpha" : alphaType == MaskAlpha ? "Masked Alpha" : "No Alpha");
#endif
        img = QImage(bufferPixels, bufferW, bufferH, format);
        if (themeData.invertPixels)
            img.invertPixels();

        if (hasCorrectionFactor)
            img = img.scaled(img.size() * correctionFactor, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        img.setDevicePixelRatio(additionalDevicePixelRatio);
    }

    // Blitting backing store
    bool useRegion = partIsTransparent && !hasAlpha && !wasAlphaSwapped;

    QRegion newRegion;
    QRegion oldRegion;
    if (useRegion) {
        newRegion = region(themeData);
        oldRegion = painter->clipRegion();
        painter->setClipRegion(newRegion);
#if defined(DEBUG_XP_STYLE) && 0
        printf("Using region:\n");
        for (const QRect &r : newRegion)
            printf("    (%d, %d, %d, %d)\n", r.x(), r.y(), r.right(), r.bottom());
#endif
    }

    if (addBorderContentClipping)
        painter->setClipRegion(scaleRegion(extraClip, 1.0 / additionalDevicePixelRatio), Qt::IntersectClip);

    if (!themeData.mirrorHorizontally && !themeData.mirrorVertically && !themeData.rotate) {
        if (!haveCachedPixmap)
            painter->drawImage(themeData.rect, img, rect);
        else
            painter->drawPixmap(themeData.rect, cachedPixmap);
    } else {
        // This is _slow_!
        // Make a copy containing only the necessary data, and mirror
        // on all wanted axes. Then draw the copy.
        // If cached, the normal pixmap is cached, instead of caching
        // all possible orientations for each part and state.
        QImage imgCopy;
        if (!haveCachedPixmap)
            imgCopy = img.copy(rect);
        else
            imgCopy = cachedPixmap.toImage();

        if (themeData.rotate) {
            QTransform rotMatrix;
            rotMatrix.rotate(themeData.rotate);
            imgCopy = imgCopy.transformed(rotMatrix);
        }
        Qt::Orientations orient = {};
        if (themeData.mirrorHorizontally)
            orient |= Qt::Horizontal;
        if (themeData.mirrorVertically)
            orient |= Qt::Vertical;
        if (themeData.mirrorHorizontally || themeData.mirrorVertically)
            imgCopy.flip(orient);
        painter->drawImage(themeData.rect, imgCopy);
    }

    if (useRegion || addBorderContentClipping) {
        if (oldRegion.isEmpty())
            painter->setClipping(false);
        else
            painter->setClipRegion(oldRegion);
    }

    // Cache the pixmap to avoid expensive swapAlphaChannel() calls
    if (!haveCachedPixmap && w && h) {
        QPixmap pix = QPixmap::fromImage(img).copy(rect);
        QPixmapCache::insert(pixmapCacheKey, pix);
#ifdef DEBUG_XP_STYLE
        printf("+++Adding pixmap to cache, size(%d, %d), wasAlphaSwapped(%d), wasAlphaFixed(%d), name(%s)\n",
               w, h, wasAlphaSwapped, wasAlphaFixed, qPrintable(pixmapCacheKey));
#endif
    }

    // Add to theme part cache
    if (!isCached) {
        memset(static_cast<void *>(&data), 0, sizeof(data));
        data.dataValid = true;
        data.partIsTransparent = partIsTransparent;
        data.alphaType = alphaType;
        data.hasAlphaChannel = hasAlpha;
        data.wasAlphaSwapped = wasAlphaSwapped;
        data.hadInvalidAlpha = wasAlphaFixed;
        alphaCache.insert(key, data);
    }
    return true;
}

/*!
    \internal

    Animations are started at a frame that is based on the current time,
    which makes it impossible to run baseline tests with this style. Allow
    overriding through a dynamic property.
*/
QTime QWindowsVistaStylePrivate::animationTime() const
{
    Q_Q(const QWindowsVistaStyle);
    static bool animationTimeOverride = q->dynamicPropertyNames().contains("_qt_animation_time");
    if (animationTimeOverride)
        return q->property("_qt_animation_time").toTime();
    return QTime::currentTime();
}

/* \internal
    Checks and returns the style object
*/
inline QObject *styleObject(const QStyleOption *option) {
    return option ? option->styleObject : nullptr;
}

/* \internal
    Checks if we can animate on a style option
*/
bool canAnimate(const QStyleOption *option) {
    return option
            && option->styleObject
            && !option->styleObject->property("_q_no_animation").toBool();
}

static inline QImage createAnimationBuffer(const QStyleOption *option, const QWidget *widget)
{
    const qreal devicePixelRatio = widget
            ? widget->devicePixelRatioF() : qApp->devicePixelRatio();
    QImage result(option->rect.size() * devicePixelRatio, QImage::Format_ARGB32_Premultiplied);
    result.setDevicePixelRatio(devicePixelRatio);
    result.fill(0);
    return result;
}

/* \internal
    Used by animations to clone a styleoption and shift its offset
*/
QStyleOption *clonedAnimationStyleOption(const QStyleOption*option) {
    QStyleOption *styleOption = nullptr;
    if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider*>(option))
        styleOption = new QStyleOptionSlider(*slider);
    else if (const QStyleOptionSpinBox *spinbox = qstyleoption_cast<const QStyleOptionSpinBox*>(option))
        styleOption = new QStyleOptionSpinBox(*spinbox);
    else if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox*>(option))
        styleOption = new QStyleOptionGroupBox(*groupBox);
    else if (const QStyleOptionComboBox *combo = qstyleoption_cast<const QStyleOptionComboBox*>(option))
        styleOption = new QStyleOptionComboBox(*combo);
    else if (const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton*>(option))
        styleOption = new QStyleOptionButton(*button);
    else
        styleOption = new QStyleOption(*option);
    styleOption->rect = QRect(QPoint(0,0), option->rect.size());
    return styleOption;
}

/* \internal
    Used by animations to delete cloned styleoption
*/
void deleteClonedAnimationStyleOption(const QStyleOption *option)
{
    if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider*>(option))
        delete slider;
    else if (const QStyleOptionSpinBox *spinbox = qstyleoption_cast<const QStyleOptionSpinBox*>(option))
        delete spinbox;
    else if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox*>(option))
        delete groupBox;
    else if (const QStyleOptionComboBox *combo = qstyleoption_cast<const QStyleOptionComboBox*>(option))
        delete combo;
    else if (const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton*>(option))
        delete button;
    else
        delete option;
}

static void populateTitleBarButtonTheme(const QStyle *proxy, const QWidget *widget,
                                        const QStyleOptionComplex *option,
                                        QStyle::SubControl subControl,
                                        bool isTitleBarActive, int part,
                                        QWindowsThemeData *theme)
{
    theme->rect = proxy->subControlRect(QStyle::CC_TitleBar, option, subControl, widget);
    theme->partId = part;
    if (widget && !widget->isEnabled())
        theme->stateId = RBS_DISABLED;
    else if (option->activeSubControls == subControl && option->state.testFlag(QStyle::State_Sunken))
        theme->stateId = RBS_PUSHED;
    else if (option->activeSubControls == subControl && option->state.testFlag(QStyle::State_MouseOver))
        theme->stateId = RBS_HOT;
    else if (!isTitleBarActive)
        theme->stateId = RBS_INACTIVE;
    else
        theme->stateId = RBS_NORMAL;
}

#if QT_CONFIG(mdiarea)
// Helper for drawing MDI buttons into the corner widget of QMenuBar in case a
// QMdiSubWindow is maximized.
static void populateMdiButtonTheme(const QStyle *proxy, const QWidget *widget,
                                   const QStyleOptionComplex *option,
                                   QStyle::SubControl subControl, int part,
                                   QWindowsThemeData *theme)
{
    theme->partId = part;
    theme->rect = proxy->subControlRect(QStyle::CC_MdiControls, option, subControl, widget);
    if (!option->state.testFlag(QStyle::State_Enabled))
        theme->stateId = CBS_INACTIVE;
    else if (option->state.testFlag(QStyle::State_Sunken) && option->activeSubControls.testFlag(subControl))
        theme->stateId = CBS_PUSHED;
    else if (option->state.testFlag(QStyle::State_MouseOver) && option->activeSubControls.testFlag(subControl))
        theme->stateId = CBS_HOT;
    else
        theme->stateId = CBS_NORMAL;
}

// Calculate an small (max 2), empirical correction factor for scaling up
// WP_MDICLOSEBUTTON, WP_MDIRESTOREBUTTON, WP_MDIMINBUTTON, which are too
// small on High DPI screens (QTBUG-75927).
static qreal mdiButtonCorrectionFactor(QWindowsThemeData &theme, const QPaintDevice *pd = nullptr)
{
    const auto dpr = pd ? pd->devicePixelRatio() : qApp->devicePixelRatio();
    const QSizeF nativeSize = QSizeF(theme.size()) / dpr;
    const QSizeF requestedSize(theme.rect.size());
    const auto rawFactor = qMin(requestedSize.width() / nativeSize.width(),
                                requestedSize.height() / nativeSize.height());
    const auto factor = rawFactor >= qreal(2) ? qreal(2) : qreal(1);
    return factor;
}
#endif // QT_CONFIG(mdiarea)

/*
  This function is used by subControlRect to check if a button
  should be drawn for the given subControl given a set of window flags.
*/
static bool buttonVisible(const QStyle::SubControl sc, const QStyleOptionTitleBar *tb){

    bool isMinimized = tb->titleBarState & Qt::WindowMinimized;
    bool isMaximized = tb->titleBarState & Qt::WindowMaximized;
    const auto flags = tb->titleBarFlags;
    bool retVal = false;
    switch (sc) {
    case QStyle::SC_TitleBarContextHelpButton:
        if (flags & Qt::WindowContextHelpButtonHint)
            retVal = true;
        break;
    case QStyle::SC_TitleBarMinButton:
        if (!isMinimized && (flags & Qt::WindowMinimizeButtonHint))
            retVal = true;
        break;
    case QStyle::SC_TitleBarNormalButton:
        if (isMinimized && (flags & Qt::WindowMinimizeButtonHint))
            retVal = true;
        else if (isMaximized && (flags & Qt::WindowMaximizeButtonHint))
            retVal = true;
        break;
    case QStyle::SC_TitleBarMaxButton:
        if (!isMaximized && (flags & Qt::WindowMaximizeButtonHint))
            retVal = true;
        break;
    case QStyle::SC_TitleBarShadeButton:
        if (!isMinimized &&  flags & Qt::WindowShadeButtonHint)
            retVal = true;
        break;
    case QStyle::SC_TitleBarUnshadeButton:
        if (isMinimized && flags & Qt::WindowShadeButtonHint)
            retVal = true;
        break;
    case QStyle::SC_TitleBarCloseButton:
        if (flags & Qt::WindowSystemMenuHint)
            retVal = true;
        break;
    case QStyle::SC_TitleBarSysMenu:
        if (flags & Qt::WindowSystemMenuHint)
            retVal = true;
        break;
    default :
        retVal = true;
    }
    return retVal;
}

//convert Qt state flags to uxtheme button states
static int buttonStateId(int flags, int partId)
{
    int stateId = 0;
    if (partId == BP_RADIOBUTTON || partId == BP_CHECKBOX) {
        if (!(flags & QStyle::State_Enabled))
            stateId = RBS_UNCHECKEDDISABLED;
        else if (flags & QStyle::State_Sunken)
            stateId = RBS_UNCHECKEDPRESSED;
        else if (flags & QStyle::State_MouseOver)
            stateId = RBS_UNCHECKEDHOT;
        else
            stateId = RBS_UNCHECKEDNORMAL;

        if (flags & QStyle::State_On)
            stateId += RBS_CHECKEDNORMAL-1;

    } else if (partId == BP_PUSHBUTTON) {
        if (!(flags & QStyle::State_Enabled))
            stateId = PBS_DISABLED;
        else if (flags & (QStyle::State_Sunken | QStyle::State_On))
            stateId = PBS_PRESSED;
        else if (flags & QStyle::State_MouseOver)
            stateId = PBS_HOT;
        else
            stateId = PBS_NORMAL;
    } else {
        Q_ASSERT(1);
    }
    return stateId;
}

static inline bool supportsStateTransition(QStyle::PrimitiveElement element,
                                           const QStyleOption *option,
                                           const QWidget *widget)
{
    bool result = false;
    switch (element) {
    case QStyle::PE_IndicatorRadioButton:
    case QStyle::PE_IndicatorCheckBox:
        result = true;
        break;
        // QTBUG-40634, do not animate when color is set in palette for PE_PanelLineEdit.
    case QStyle::PE_FrameLineEdit:
        result = !QWindowsVistaStylePrivate::isLineEditBaseColorSet(option, widget);
        break;
    default:
        break;
    }
    return result;
}

/*!
  \class QWindowsVistaStyle
  \brief The QWindowsVistaStyle class provides a look and feel suitable for applications on Microsoft Windows Vista.
  \since 4.3
  \ingroup appearance
  \inmodule QtWidgets
  \internal

  \warning This style is only available on the Windows Vista platform
  because it makes use of Windows Vista's style engine.

  \sa QMacStyle, QFusionStyle
*/

/*!
  Constructs a QWindowsVistaStyle object.
*/
QWindowsVistaStyle::QWindowsVistaStyle() : QWindowsVistaStyle(*new QWindowsVistaStylePrivate)
{
}

/*!
 \internal
 Constructs a QWindowsStyle object.
*/
QWindowsVistaStyle::QWindowsVistaStyle(QWindowsVistaStylePrivate &dd) : QWindowsStyle(dd)
{
}

/*!
  Destructor.
*/
QWindowsVistaStyle::~QWindowsVistaStyle() = default;


/*!
 \internal

  Animations are used for some state transitions on specific widgets.

  Only one running animation can exist for a widget at any specific
  time.  Animations can be added through
  QWindowsVistaStylePrivate::startAnimation(Animation *) and any
  existing animation on a widget can be retrieved with
  QWindowsVistaStylePrivate::widgetAnimation(Widget *).

  Once an animation has been started,
  QWindowsVistaStylePrivate::timerEvent(QTimerEvent *) will
  continuously call update() on the widget until it is stopped,
  meaning that drawPrimitive will be called many times until the
  transition has completed. During this time, the result will be
  retrieved by the Animation::paint(...) function and not by the style
  itself.

  To determine if a transition should occur, the style needs to know
  the previous state of the widget as well as the current one. This is
  solved by updating dynamic properties on the widget every time the
  function is called.

  Transitions interrupting existing transitions should always be
  smooth, so whenever a hover-transition is started on a pulsating
  button, it uses the current frame of the pulse-animation as the
  starting image for the hover transition.

 */
void QWindowsVistaStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                                       QPainter *painter, const QWidget *widget) const
{
    if (!QWindowsVistaStylePrivate::useVista()) {
        QWindowsStyle::drawPrimitive(element, option, painter, widget);
        return;
    }

    QWindowsVistaStylePrivate *d = const_cast<QWindowsVistaStylePrivate*>(d_func());

    int state = option->state;
    QRect rect = option->rect;

    if ((state & State_Enabled) && d->transitionsEnabled() && canAnimate(option)) {
        if (supportsStateTransition(element, option, widget)) {
            // Retrieve and update the dynamic properties tracking
            // the previous state of the widget:
            QObject *styleObject = option->styleObject;
            styleObject->setProperty("_q_no_animation", true);
            int oldState = styleObject->property("_q_stylestate").toInt();
            QRect oldRect = styleObject->property("_q_stylerect").toRect();
            QRect newRect = rect;
            styleObject->setProperty("_q_stylestate", int(option->state));
            styleObject->setProperty("_q_stylerect", option->rect);

            bool doTransition = oldState &&
                    ((state & State_Sunken)    != (oldState & State_Sunken) ||
                    (state & State_On)         != (oldState & State_On)     ||
                    (state & State_MouseOver)  != (oldState & State_MouseOver));

            if (oldRect != newRect ||
                    (state & State_Enabled) != (oldState & State_Enabled) ||
                    (state & State_Active)  != (oldState & State_Active))
                d->stopAnimation(styleObject);

            if (state & State_ReadOnly && element == PE_FrameLineEdit) // Do not animate read only line edits
                doTransition = false;

            if (doTransition) {
                QStyleOption *styleOption = clonedAnimationStyleOption(option);
                styleOption->state = QStyle::State(oldState);

                QWindowsVistaAnimation *animate = qobject_cast<QWindowsVistaAnimation *>(d->animation(styleObject));
                QWindowsVistaTransition *transition = new QWindowsVistaTransition(styleObject);

                // We create separate images for the initial and final transition states and store them in the
                // Transition object.
                QImage startImage = createAnimationBuffer(option, widget);
                QPainter startPainter(&startImage);

                QImage endImage = createAnimationBuffer(option, widget);
                QPainter endPainter(&endImage);

                // If we have a running animation on the widget already, we will use that to paint the initial
                // state of the new transition, this ensures a smooth transition from a current animation such as a
                // pulsating default button into the intended target state.
                if (!animate)
                    proxy()->drawPrimitive(element, styleOption, &startPainter, widget);
                else
                    animate->paint(&startPainter, styleOption);

                transition->setStartImage(startImage);

                // The end state of the transition is simply the result we would have painted
                // if the style was not animated.
                styleOption->styleObject = nullptr;
                styleOption->state = option->state;
                proxy()->drawPrimitive(element, styleOption, &endPainter, widget);

                transition->setEndImage(endImage);

                HTHEME theme;
                int partId;
                DWORD duration;
                int fromState = 0;
                int toState = 0;

                //translate state flags to UXTHEME states :
                if (element == PE_FrameLineEdit) {
                    theme = OpenThemeData(nullptr, L"Edit");
                    partId = EP_EDITBORDER_NOSCROLL;

                    if (oldState & State_HasFocus)
                        fromState = ETS_SELECTED;
                    else if (oldState & State_MouseOver)
                        fromState = ETS_HOT;
                    else
                        fromState = ETS_NORMAL;

                    if (state & State_HasFocus)
                        toState = ETS_SELECTED;
                    else if (state & State_MouseOver)
                        toState = ETS_HOT;
                    else
                        toState = ETS_NORMAL;

                } else {
                    theme = OpenThemeData(nullptr, L"Button");
                    if (element == PE_IndicatorRadioButton)
                        partId = BP_RADIOBUTTON;
                    else if (element == PE_IndicatorCheckBox)
                        partId = BP_CHECKBOX;
                    else
                        partId = BP_PUSHBUTTON;

                    fromState = buttonStateId(oldState, partId);
                    toState = buttonStateId(option->state, partId);
                }

                // Retrieve the transition time between the states from the system.
                if (theme
                        && SUCCEEDED(GetThemeTransitionDuration(theme, partId, fromState, toState,
                                                                TMT_TRANSITIONDURATIONS, &duration))) {
                    transition->setDuration(int(duration));
                }
                transition->setStartTime(d->animationTime());

                deleteClonedAnimationStyleOption(styleOption);
                d->startAnimation(transition);
            }
            styleObject->setProperty("_q_no_animation", false);
        }
    }

    int themeNumber = -1;
    int partId = 0;
    int stateId = 0;
    bool hMirrored = false;
    bool vMirrored = false;
    bool noBorder = false;
    bool noContent = false;
    int  rotate = 0;

    switch (element) {
    case PE_PanelButtonCommand:
        if (const auto *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            QBrush fill;
            if (!(state & State_Sunken) && (state & State_On))
                fill = QBrush(option->palette.light().color(), Qt::Dense4Pattern);
            else
                fill = option->palette.brush(QPalette::Button);
            if (btn->features & QStyleOptionButton::DefaultButton && state & State_Sunken) {
                painter->setPen(option->palette.dark().color());
                painter->setBrush(fill);
                painter->drawRect(rect.adjusted(0, 0, -1, -1));
            } else if (state & (State_Raised | State_On | State_Sunken)) {
                qDrawWinButton(painter, rect, option->palette, state & (State_Sunken | State_On),
                               &fill);
            } else {
                painter->fillRect(rect, fill);
            }
        }
        break;

    case PE_PanelButtonTool:
#if QT_CONFIG(dockwidget)
        if (widget && widget->inherits("QDockWidgetTitleButton")) {
            if (const QWidget *dw = widget->parentWidget())
                if (dw->isWindow()) {
                    return;
                }
        }
#endif // QT_CONFIG(dockwidget)
        themeNumber = QWindowsVistaStylePrivate::ToolBarTheme;
        partId = TP_BUTTON;
        if (!(option->state & State_Enabled))
            stateId = TS_DISABLED;
        else if (option->state & State_Sunken)
            stateId = TS_PRESSED;
        else if (option->state & State_MouseOver)
            stateId = option->state & State_On ? TS_HOTCHECKED : TS_HOT;
        else if (option->state & State_On)
            stateId = TS_CHECKED;
        else if (!(option->state & State_AutoRaise))
            stateId = TS_HOT;
        else
            stateId = TS_NORMAL;

        break;

    case PE_IndicatorHeaderArrow:
        if (const auto *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            int stateId = HSAS_SORTEDDOWN;
            if (header->sortIndicator & QStyleOptionHeader::SortDown)
                stateId = HSAS_SORTEDUP; //note that the uxtheme sort down indicator is the inverse of ours
            QWindowsThemeData theme(widget, painter,
                            QWindowsVistaStylePrivate::HeaderTheme,
                            HP_HEADERSORTARROW, stateId, option->rect);
            d->drawBackground(theme);
            return;
        }
        break;

    case PE_IndicatorCheckBox:
        if (auto *animate =
                qobject_cast<QWindowsVistaAnimation *>(d->animation(styleObject(option)))) {
            animate->paint(painter, option);
            return;
        } else {
            themeNumber = QWindowsVistaStylePrivate::ButtonTheme;
            partId = BP_CHECKBOX;

            if (!(option->state & State_Enabled))
                stateId = CBS_UNCHECKEDDISABLED;
            else if (option->state & State_Sunken)
                stateId = CBS_UNCHECKEDPRESSED;
            else if (option->state & State_MouseOver)
                stateId = CBS_UNCHECKEDHOT;
            else
                stateId = CBS_UNCHECKEDNORMAL;

            if (option->state & State_On)
                stateId += CBS_CHECKEDNORMAL-1;
            else if (option->state & State_NoChange)
                stateId += CBS_MIXEDNORMAL-1;
        }
        break;

    case PE_IndicatorItemViewItemCheck: {
        QStyleOptionButton button;
        button.QStyleOption::operator=(*option);
        button.state &= ~State_MouseOver;
        proxy()->drawPrimitive(PE_IndicatorCheckBox, &button, painter, widget);
        return;
    }

    case PE_IndicatorBranch: {
        QWindowsThemeData theme(widget, painter, QWindowsVistaStylePrivate::VistaTreeViewTheme);
        static int decoration_size = 0;
        if (!decoration_size && theme.isValid()) {
            QWindowsThemeData themeSize = theme;
            themeSize.partId = TVP_HOTGLYPH;
            themeSize.stateId = GLPS_OPENED;
            const QSizeF size = themeSize.size() * QWindowsStylePrivate::nativeMetricScaleFactor(widget);
            decoration_size = qRound(qMax(size.width(), size.height()));
        }
        int mid_h = option->rect.x() + option->rect.width() / 2;
        int mid_v = option->rect.y() + option->rect.height() / 2;
        if (option->state & State_Children) {
            int delta = decoration_size / 2;
            theme.rect = QRect(mid_h - delta, mid_v - delta, decoration_size, decoration_size);
            theme.partId = option->state & State_MouseOver ? TVP_HOTGLYPH : TVP_GLYPH;
            theme.stateId = option->state & QStyle::State_Open ? GLPS_OPENED : GLPS_CLOSED;
            if (option->direction == Qt::RightToLeft)
                theme.mirrorHorizontally = true;
            d->drawBackground(theme);
        }
        return;
    }

    case PE_PanelButtonBevel:
        if (QWindowsVistaAnimation *animate =
                qobject_cast<QWindowsVistaAnimation *>(d->animation(styleObject(option)))) {
            animate->paint(painter, option);
            return;
        }

        themeNumber = QWindowsVistaStylePrivate::ButtonTheme;
        partId = BP_PUSHBUTTON;
        if (!(option->state & State_Enabled))
            stateId = PBS_DISABLED;
        else if ((option->state & State_Sunken) || (option->state & State_On))
            stateId = PBS_PRESSED;
        else if (option->state & State_MouseOver)
            stateId = PBS_HOT;
        else
            stateId = PBS_NORMAL;
        break;

    case PE_IndicatorRadioButton:
        if (QWindowsVistaAnimation *animate =
                qobject_cast<QWindowsVistaAnimation *>(d->animation(styleObject(option)))) {
            animate->paint(painter, option);
            return;
        } else {
            themeNumber = QWindowsVistaStylePrivate::ButtonTheme;
            partId = BP_RADIOBUTTON;

            if (!(option->state & State_Enabled))
                stateId = RBS_UNCHECKEDDISABLED;
            else if (option->state & State_Sunken)
                stateId = RBS_UNCHECKEDPRESSED;
            else if (option->state & State_MouseOver)
                stateId = RBS_UNCHECKEDHOT;
            else
                stateId = RBS_UNCHECKEDNORMAL;

            if (option->state & State_On)
                stateId += RBS_CHECKEDNORMAL-1;
        }
        break;

    case PE_Frame:
#if QT_CONFIG(accessibility)
        if (QStyleHelper::isInstanceOf(option->styleObject, QAccessible::EditableText)
             || QStyleHelper::isInstanceOf(option->styleObject, QAccessible::StaticText) ||
#else
        if (
#endif
                (widget && widget->inherits("QTextEdit"))) {
            painter->save();
            int stateId = ETS_NORMAL;
            if (!(state & State_Enabled))
                stateId = ETS_DISABLED;
            else if (state & State_ReadOnly)
                stateId = ETS_READONLY;
            else if (state & State_HasFocus)
                stateId = ETS_SELECTED;
            QWindowsThemeData theme(widget, painter,
                            QWindowsVistaStylePrivate::EditTheme,
                            EP_EDITBORDER_HVSCROLL, stateId, option->rect);
            // Since EP_EDITBORDER_HVSCROLL does not us borderfill, theme.noContent cannot be used for clipping
            int borderSize = 1;
            GetThemeInt(theme.handle(), theme.partId, theme.stateId, TMT_BORDERSIZE, &borderSize);
            QRegion clipRegion = option->rect;
            QRegion content = option->rect.adjusted(borderSize, borderSize, -borderSize, -borderSize);
            clipRegion ^= content;
            painter->setClipRegion(clipRegion);
            d->drawBackground(theme);
            painter->restore();
            return;
        } else {
            if (option->state & State_Raised)
                return;

            themeNumber = QWindowsVistaStylePrivate::ListViewTheme;
            partId = LVP_LISTGROUP;
            QWindowsThemeData theme(widget, nullptr, themeNumber, partId);

            if (!(option->state & State_Enabled))
                stateId = ETS_DISABLED;
            else
                stateId = ETS_NORMAL;

            int fillType;

            if (GetThemeEnumValue(theme.handle(), partId, stateId, TMT_BGTYPE, &fillType) == S_OK) {
                if (fillType == BT_BORDERFILL) {
                    COLORREF bcRef;
                    GetThemeColor(theme.handle(), partId, stateId, TMT_BORDERCOLOR, &bcRef);
                    QColor bordercolor(qRgb(GetRValue(bcRef), GetGValue(bcRef), GetBValue(bcRef)));
                    QPen oldPen = painter->pen();

                    // Inner white border
                    painter->setPen(QPen(option->palette.base().color(), 0));
                    const qreal dpi = QStyleHelper::dpi(option);
                    const auto topLevelAdjustment = QStyleHelper::dpiScaled(0.5, dpi);
                    const auto bottomRightAdjustment = QStyleHelper::dpiScaled(-1, dpi);
                    painter->drawRect(QRectF(option->rect).adjusted(topLevelAdjustment, topLevelAdjustment,
                                                                    bottomRightAdjustment, bottomRightAdjustment));
                    // Outer dark border
                    painter->setPen(QPen(bordercolor, 0));
                    painter->drawRect(QRectF(option->rect).adjusted(0, 0, -topLevelAdjustment, -topLevelAdjustment));
                    painter->setPen(oldPen);
                }

                if (fillType == BT_BORDERFILL || fillType == BT_NONE)
                    return;
            }
        }
        break;

    case PE_FrameMenu: {
        int stateId = option->state & State_Active ? MB_ACTIVE : MB_INACTIVE;
        QWindowsThemeData theme(widget, painter,
                        QWindowsVistaStylePrivate::MenuTheme,
                        MENU_POPUPBORDERS, stateId, option->rect);
        d->drawBackground(theme);
        return;
    }

    case PE_PanelMenuBar:
        break;

#if QT_CONFIG(dockwidget)
    case PE_IndicatorDockWidgetResizeHandle:
        return;

    case PE_FrameDockWidget:
        if (const auto *frm = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            themeNumber = QWindowsVistaStylePrivate::WindowTheme;
            if (option->state & State_Active)
                stateId = FS_ACTIVE;
            else
                stateId = FS_INACTIVE;

            int fwidth = proxy()->pixelMetric(PM_DockWidgetFrameWidth, frm, widget);

            QWindowsThemeData theme(widget, painter, themeNumber, 0, stateId);

            if (!theme.isValid())
                break;

            theme.rect = QRect(frm->rect.x(), frm->rect.y(), frm->rect.x()+fwidth, frm->rect.height()-fwidth);
            theme.partId = WP_SMALLFRAMELEFT;
            d->drawBackground(theme);
            theme.rect = QRect(frm->rect.width()-fwidth, frm->rect.y(), fwidth, frm->rect.height()-fwidth);
            theme.partId = WP_SMALLFRAMERIGHT;
            d->drawBackground(theme);
            theme.rect = QRect(frm->rect.x(), frm->rect.bottom()-fwidth+1, frm->rect.width(), fwidth);
            theme.partId = WP_SMALLFRAMEBOTTOM;
            d->drawBackground(theme);
            return;
        }
        break;
#endif // QT_CONFIG(dockwidget)

    case PE_FrameTabWidget:
#if QT_CONFIG(tabwidget)
        if (const auto *tab = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(option)) {
            themeNumber = QWindowsVistaStylePrivate::TabTheme;
            partId = TABP_PANE;

            if (widget) {
                bool useGradient = true;
                const int maxlength = 256;
                wchar_t themeFileName[maxlength];
                wchar_t themeColor[maxlength];
                // Due to a a scaling issue with the XP Silver theme, tab gradients are not used with it
                if (GetCurrentThemeName(themeFileName, maxlength, themeColor, maxlength, nullptr, 0) == S_OK) {
                    wchar_t *offset = nullptr;
                    if ((offset = wcsrchr(themeFileName, QChar(QLatin1Char('\\')).unicode())) != nullptr) {
                        offset++;
                        if (!lstrcmp(offset, L"Luna.msstyles") && !lstrcmp(offset, L"Metallic"))
                            useGradient = false;
                    }
                }
                // This should work, but currently there's an error in the ::drawBackgroundDirectly()
                // code, when using the HDC directly..
                if (useGradient) {
                    QStyleOptionTabWidgetFrame frameOpt = *tab;
                    frameOpt.rect = widget->rect();
                    QRect contentsRect = subElementRect(SE_TabWidgetTabContents, &frameOpt, widget);
                    QRegion reg = option->rect;
                    reg -= contentsRect;
                    painter->setClipRegion(reg);
                    QWindowsThemeData theme(widget, painter, themeNumber, partId, stateId, rect);
                    theme.mirrorHorizontally = hMirrored;
                    theme.mirrorVertically = vMirrored;
                    d->drawBackground(theme);
                    painter->setClipRect(contentsRect);
                    partId = TABP_BODY;
                }
            }
            switch (tab->shape) {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
                break;
            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
                vMirrored = true;
                break;
            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
                rotate = 90;
                break;
            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
                rotate = 90;
                hMirrored = true;
                break;
            default:
                break;
            }
        }
        break;
#endif  // QT_CONFIG(tabwidget)
    case PE_FrameStatusBarItem:
        themeNumber = QWindowsVistaStylePrivate::StatusTheme;
        partId = SP_PANE;
        break;

    case PE_FrameWindow:
        if (const auto *frm = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            themeNumber = QWindowsVistaStylePrivate::WindowTheme;
            if (option->state & State_Active)
                stateId = FS_ACTIVE;
            else
                stateId = FS_INACTIVE;

            int fwidth = int((frm->lineWidth + frm->midLineWidth) / QWindowsStylePrivate::nativeMetricScaleFactor(widget));

            QWindowsThemeData theme(widget, painter, themeNumber, 0, stateId);
            if (!theme.isValid())
                break;

            // May fail due to too-large buffers for large widgets, fall back to Windows style.
            theme.rect = QRect(option->rect.x(), option->rect.y()+fwidth, option->rect.x()+fwidth, option->rect.height()-fwidth);
            theme.partId = WP_FRAMELEFT;
            if (!d->drawBackground(theme)) {
                QWindowsStyle::drawPrimitive(element, option, painter, widget);
                return;
            }
            theme.rect = QRect(option->rect.width()-fwidth, option->rect.y()+fwidth, fwidth, option->rect.height()-fwidth);
            theme.partId = WP_FRAMERIGHT;
            if (!d->drawBackground(theme)) {
                QWindowsStyle::drawPrimitive(element, option, painter, widget);
                return;
            }
            theme.rect = QRect(option->rect.x(), option->rect.height()-fwidth, option->rect.width(), fwidth);
            theme.partId = WP_FRAMEBOTTOM;
            if (!d->drawBackground(theme)) {
                QWindowsStyle::drawPrimitive(element, option, painter, widget);
                return;
            }
            theme.rect = QRect(option->rect.x(), option->rect.y(), option->rect.width(), option->rect.y()+fwidth);
            theme.partId = WP_CAPTION;
            if (!d->drawBackground(theme))
                QWindowsStyle::drawPrimitive(element, option, painter, widget);
            return;
        }
        break;

    case PE_PanelLineEdit:
        if (const auto *panel = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            bool isEnabled = state & State_Enabled;
            if (QWindowsVistaStylePrivate::isLineEditBaseColorSet(option, widget)) {
                painter->fillRect(panel->rect, panel->palette.brush(QPalette::Base));
            } else {
                int partId = EP_BACKGROUND;
                int stateId = EBS_NORMAL;
                if (!isEnabled)
                    stateId = EBS_DISABLED;
                else if (option->state & State_ReadOnly)
                    stateId = EBS_READONLY;
                else if (option->state & State_MouseOver)
                    stateId = EBS_HOT;

                QWindowsThemeData theme(nullptr, painter, QWindowsVistaStylePrivate::EditTheme,
                                partId, stateId, rect);
                if (!theme.isValid()) {
                    QWindowsStyle::drawPrimitive(element, option, painter, widget);
                    return;
                }
                int bgType;
                GetThemeEnumValue(theme.handle(), partId, stateId, TMT_BGTYPE, &bgType);
                if (bgType == BT_IMAGEFILE) {
                    d->drawBackground(theme);
                } else {
                    QBrush fillColor = option->palette.brush(QPalette::Base);
                    if (!isEnabled) {
                        PROPERTYORIGIN origin = PO_NOTFOUND;
                        GetThemePropertyOrigin(theme.handle(), theme.partId, theme.stateId, TMT_FILLCOLOR, &origin);
                        // Use only if the fill property comes from our part
                        if ((origin == PO_PART || origin == PO_STATE)) {
                            COLORREF bgRef;
                            GetThemeColor(theme.handle(), partId, stateId, TMT_FILLCOLOR, &bgRef);
                            fillColor = QBrush(qRgb(GetRValue(bgRef), GetGValue(bgRef), GetBValue(bgRef)));
                        }
                    }
                    painter->fillRect(option->rect, fillColor);
                }
            }
            if (panel->lineWidth > 0)
                proxy()->drawPrimitive(PE_FrameLineEdit, panel, painter, widget);
        }
        return;

    case PE_IndicatorButtonDropDown:
        themeNumber = QWindowsVistaStylePrivate::ToolBarTheme;
        partId = TP_SPLITBUTTONDROPDOWN;
        if (!(option->state & State_Enabled))
            stateId = TS_DISABLED;
        else if (option->state & State_Sunken)
            stateId = TS_PRESSED;
        else if (option->state & State_MouseOver)
            stateId = option->state & State_On ? TS_HOTCHECKED : TS_HOT;
        else if (option->state & State_On)
            stateId = TS_CHECKED;
        else if (!(option->state & State_AutoRaise))
            stateId = TS_HOT;
        else
            stateId = TS_NORMAL;
        if (option->direction == Qt::RightToLeft)
            hMirrored = true;
        break;

    case PE_FrameLineEdit:
        if (QWindowsVistaAnimation *animate = qobject_cast<QWindowsVistaAnimation *>(d->animation(styleObject(option)))) {
            animate->paint(painter, option);
        } else {
            if (QWindowsVistaStylePrivate::isItemViewDelegateLineEdit(widget)) {
                // we try to check if this lineedit is a delegate on a QAbstractItemView-derived class.
                QPen oldPen = painter->pen();
                // Inner white border
                painter->setPen(option->palette.base().color());
                painter->drawRect(option->rect.adjusted(1, 1, -2, -2));
                // Outer dark border
                painter->setPen(option->palette.shadow().color());
                painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
                painter->setPen(oldPen);
                return;
            }
            int stateId = ETS_NORMAL;
            if (!(state & State_Enabled))
                stateId = ETS_DISABLED;
            else if (state & State_ReadOnly)
                stateId = ETS_READONLY;
            else if (state & State_HasFocus)
                stateId = ETS_SELECTED;
            else if (state & State_MouseOver)
                stateId = ETS_HOT;
            QWindowsThemeData theme(widget, painter,
                            QWindowsVistaStylePrivate::EditTheme,
                            EP_EDITBORDER_NOSCROLL, stateId, option->rect);
            theme.noContent = true;
            painter->save();
            QRegion clipRegion = option->rect;
            clipRegion -= option->rect.adjusted(2, 2, -2, -2);
            painter->setClipRegion(clipRegion);
            d->drawBackground(theme);
            painter->restore();
        }
        return;

    case PE_FrameGroupBox:
        themeNumber = QWindowsVistaStylePrivate::ButtonTheme;
        partId = BP_GROUPBOX;
        if (!(option->state & State_Enabled))
            stateId = GBS_DISABLED;
        else
            stateId = GBS_NORMAL;
        if (const auto *frame = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            if (frame->features & QStyleOptionFrame::Flat) {
                // Windows XP does not have a theme part for a flat GroupBox, paint it with the windows style
                QRect fr = frame->rect;
                QPoint p1(fr.x(), fr.y() + 1);
                QPoint p2(fr.x() + fr.width(), p1.y() + 1);
                rect = QRect(p1, p2);
                themeNumber = -1;
            }
        }
        break;

    case PE_IndicatorToolBarHandle: {
        QWindowsThemeData theme;
        QRect rect;
        if (option->state & State_Horizontal) {
            theme = QWindowsThemeData(widget, painter,
                              QWindowsVistaStylePrivate::RebarTheme,
                              RP_GRIPPER, ETS_NORMAL, option->rect.adjusted(0, 1, -2, -2));
            rect = option->rect.adjusted(0, 1, 0, -2);
            rect.setWidth(4);
        } else {
            theme = QWindowsThemeData(widget, painter, QWindowsVistaStylePrivate::RebarTheme,
                              RP_GRIPPERVERT, ETS_NORMAL, option->rect.adjusted(0, 1, -2, -2));
            rect = option->rect.adjusted(1, 0, -1, 0);
            rect.setHeight(4);
        }
        theme.rect = rect;
        d->drawBackground(theme);
        return;
    }

    case PE_IndicatorToolBarSeparator: {
        QPen pen = painter->pen();
        int margin = 3;
        painter->setPen(option->palette.window().color().darker(114));
        if (option->state & State_Horizontal) {
            int x1 = option->rect.center().x();
            painter->drawLine(QPoint(x1, option->rect.top() + margin), QPoint(x1, option->rect.bottom() - margin));
        } else {
            int y1 = option->rect.center().y();
            painter->drawLine(QPoint(option->rect.left() + margin, y1), QPoint(option->rect.right() - margin, y1));
        }
        painter->setPen(pen);
        return;
    }

    case PE_PanelTipLabel: {
        QWindowsThemeData theme(widget, painter,
                        QWindowsVistaStylePrivate::ToolTipTheme,
                        TTP_STANDARD, TTSS_NORMAL, option->rect);
        d->drawBackground(theme);
        return;
    }

    case PE_FrameTabBarBase:
#if QT_CONFIG(tabbar)
        if (const auto *tbb = qstyleoption_cast<const QStyleOptionTabBarBase *>(option)) {
            painter->save();
            switch (tbb->shape) {
            case QTabBar::RoundedNorth:
                painter->setPen(QPen(tbb->palette.dark(), 0));
                painter->drawLine(tbb->rect.topLeft(), tbb->rect.topRight());
                break;
            case QTabBar::RoundedWest:
                painter->setPen(QPen(tbb->palette.dark(), 0));
                painter->drawLine(tbb->rect.left(), tbb->rect.top(), tbb->rect.left(), tbb->rect.bottom());
                break;
            case QTabBar::RoundedSouth:
                painter->setPen(QPen(tbb->palette.dark(), 0));
                painter->drawLine(tbb->rect.left(), tbb->rect.top(),
                                  tbb->rect.right(), tbb->rect.top());
                break;
            case QTabBar::RoundedEast:
                painter->setPen(QPen(tbb->palette.dark(), 0));
                painter->drawLine(tbb->rect.topLeft(), tbb->rect.bottomLeft());
                break;
            case QTabBar::TriangularNorth:
            case QTabBar::TriangularEast:
            case QTabBar::TriangularWest:
            case QTabBar::TriangularSouth:
                painter->restore();
                QWindowsStyle::drawPrimitive(element, option, painter, widget);
                return;
            }
            painter->restore();
        }
#endif  // QT_CONFIG(tabbar)
        return;

    case PE_Widget: {
#if QT_CONFIG(dialogbuttonbox)
        const QDialogButtonBox *buttonBox = nullptr;
        if (qobject_cast<const QMessageBox *> (widget))
            buttonBox = widget->findChild<const QDialogButtonBox *>(QLatin1String("qt_msgbox_buttonbox"));
        if (buttonBox) {
            //draw white panel part
            QWindowsThemeData theme(widget, painter,
                            QWindowsVistaStylePrivate::TaskDialogTheme,
                            TDLG_PRIMARYPANEL, 0, option->rect);
            QRect toprect = option->rect;
            toprect.setBottom(buttonBox->geometry().top());
            theme.rect = toprect;
            d->drawBackground(theme);

            //draw bottom panel part
            QRect buttonRect = option->rect;
            buttonRect.setTop(buttonBox->geometry().top());
            theme.rect = buttonRect;
            theme.partId = TDLG_SECONDARYPANEL;
            d->drawBackground(theme);
        }
#endif
        return;
    }

    case PE_PanelItemViewItem: {
        const QStyleOptionViewItem *vopt;
        bool newStyle = true;
        QAbstractItemView::SelectionBehavior selectionBehavior = QAbstractItemView::SelectRows;
        QAbstractItemView::SelectionMode selectionMode = QAbstractItemView::NoSelection;
        if (const QAbstractItemView *view = qobject_cast<const QAbstractItemView *>(widget)) {
            newStyle = !qobject_cast<const QTableView*>(view);
            selectionBehavior = view->selectionBehavior();
            selectionMode = view->selectionMode();
#if QT_CONFIG(accessibility)
        } else if (!widget) {
            newStyle = !QStyleHelper::hasAncestor(option->styleObject, QAccessible::MenuItem) ;
#endif
        }

        if (newStyle && (vopt = qstyleoption_cast<const QStyleOptionViewItem *>(option))) {
            bool selected = vopt->state & QStyle::State_Selected;
            const bool hover = selectionMode != QAbstractItemView::NoSelection && (vopt->state & QStyle::State_MouseOver);
            bool active = vopt->state & QStyle::State_Active;

            if (vopt->features & QStyleOptionViewItem::Alternate)
                painter->fillRect(vopt->rect, vopt->palette.alternateBase());

            QPalette::ColorGroup cg = vopt->state & QStyle::State_Enabled
                    ? QPalette::Normal : QPalette::Disabled;
            if (cg == QPalette::Normal && !(vopt->state & QStyle::State_Active))
                cg = QPalette::Inactive;

            QRect itemRect = subElementRect(QStyle::SE_ItemViewItemFocusRect, option, widget).adjusted(-1, 0, 1, 0);
            itemRect.setTop(vopt->rect.top());
            itemRect.setBottom(vopt->rect.bottom());

            QSize sectionSize = itemRect.size();
            if (vopt->showDecorationSelected)
                sectionSize = vopt->rect.size();

            if (selectionBehavior == QAbstractItemView::SelectRows)
                sectionSize.setWidth(vopt->rect.width());
            QPixmap pixmap;

            if (vopt->backgroundBrush.style() != Qt::NoBrush) {
                QPainterStateGuard psg(painter);
                painter->setBrushOrigin(vopt->rect.topLeft());
                painter->fillRect(vopt->rect, vopt->backgroundBrush);
            }

            if (hover || selected) {
                if (sectionSize.width() > 0 && sectionSize.height() > 0) {
                    QString key = QStringLiteral(u"qvdelegate-%1-%2-%3-%4-%5").arg(sectionSize.width())
                            .arg(sectionSize.height()).arg(selected).arg(active).arg(hover);
                    if (!QPixmapCache::find(key, &pixmap)) {
                        pixmap = QPixmap(sectionSize);
                        pixmap.fill(Qt::transparent);

                        int state;
                        if (selected && hover)
                            state = LISS_HOTSELECTED;
                        else if (selected && !active)
                            state = LISS_SELECTEDNOTFOCUS;
                        else if (selected)
                            state = LISS_SELECTED;
                        else
                            state = LISS_HOT;

                        QPainter pixmapPainter(&pixmap);

                        QWindowsThemeData theme(widget, &pixmapPainter,
                                        QWindowsVistaStylePrivate::VistaTreeViewTheme,
                                        LVP_LISTITEM, state, QRect(0, 0, sectionSize.width(), sectionSize.height()));

                        if (!theme.isValid())
                            break;

                        d->drawBackground(theme);
                        QPixmapCache::insert(key, pixmap);
                    }
                }

                if (vopt->showDecorationSelected) {
                    const int frame = 2; //Assumes a 2 pixel pixmap border
                    QRect srcRect = QRect(0, 0, sectionSize.width(), sectionSize.height());
                    QRect pixmapRect = vopt->rect;
                    bool reverse = vopt->direction == Qt::RightToLeft;
                    bool leftSection = vopt->viewItemPosition == QStyleOptionViewItem::Beginning;
                    bool rightSection = vopt->viewItemPosition == QStyleOptionViewItem::End;
                    if (vopt->viewItemPosition == QStyleOptionViewItem::OnlyOne
                            || vopt->viewItemPosition == QStyleOptionViewItem::Invalid)
                        painter->drawPixmap(pixmapRect.topLeft(), pixmap);
                    else if (reverse ? rightSection : leftSection){
                        painter->drawPixmap(QRect(pixmapRect.topLeft(),
                                                  QSize(frame, pixmapRect.height())), pixmap,
                                            QRect(QPoint(0, 0), QSize(frame, pixmapRect.height())));
                        painter->drawPixmap(pixmapRect.adjusted(frame, 0, 0, 0),
                                            pixmap, srcRect.adjusted(frame, 0, -frame, 0));
                    } else if (reverse ? leftSection : rightSection) {
                        painter->drawPixmap(QRect(pixmapRect.topRight() - QPoint(frame - 1, 0),
                                                  QSize(frame, pixmapRect.height())), pixmap,
                                            QRect(QPoint(pixmapRect.width() - frame, 0),
                                                  QSize(frame, pixmapRect.height())));
                        painter->drawPixmap(pixmapRect.adjusted(0, 0, -frame, 0),
                                            pixmap, srcRect.adjusted(frame, 0, -frame, 0));
                    } else if (vopt->viewItemPosition == QStyleOptionViewItem::Middle)
                        painter->drawPixmap(pixmapRect, pixmap,
                                            srcRect.adjusted(frame, 0, -frame, 0));
                } else {
                    if (vopt->text.isEmpty() && vopt->icon.isNull())
                        break;
                    painter->drawPixmap(itemRect.topLeft(), pixmap);
                }
            }
            return;
        }
        break;
    }

    default:
        break;
    }

    QWindowsThemeData theme(widget, painter, themeNumber, partId, stateId, rect);

    if (!theme.isValid()) {
        QWindowsStyle::drawPrimitive(element, option, painter, widget);
        return;
    }

    theme.mirrorHorizontally = hMirrored;
    theme.mirrorVertically = vMirrored;
    theme.noBorder = noBorder;
    theme.noContent = noContent;
    theme.rotate = rotate;

    d->drawBackground(theme);
}

/*! \internal */
int QWindowsVistaStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                                  QStyleHintReturn *returnData) const
{
    QWindowsVistaStylePrivate *d = const_cast<QWindowsVistaStylePrivate*>(d_func());

    int ret = 0;
    switch (hint) {
    case SH_EtchDisabledText:
        ret = (qobject_cast<const QLabel*>(widget) != 0);
        break;

    case SH_SpinControls_DisableOnBounds:
        ret = 0;
        break;

    case SH_TitleBar_AutoRaise:
    case SH_TitleBar_NoBorder:
        ret = 1;
        break;

    case SH_GroupBox_TextLabelColor:
        if (!widget || widget->isEnabled())
            ret = d->groupBoxTextColor;
        else
            ret = d->groupBoxTextColorDisabled;
        break;

    case SH_WindowFrame_Mask: {
        ret = 1;
        auto *mask = qstyleoption_cast<QStyleHintReturnMask *>(returnData);
        const auto *titlebar = qstyleoption_cast<const QStyleOptionTitleBar *>(option);
        if (mask && titlebar) {
            // Note certain themes will not return the whole window frame but only the titlebar part when
            // queried This function needs to return the entire window mask, hence we will only fetch the mask for the
            // titlebar itself and add the remaining part of the window rect at the bottom.
            int tbHeight = proxy()->pixelMetric(PM_TitleBarHeight, option, widget);
            QRect titleBarRect = option->rect;
            titleBarRect.setHeight(tbHeight);
            QWindowsThemeData themeData;
            if (titlebar->titleBarState & Qt::WindowMinimized) {
                themeData = QWindowsThemeData(widget, nullptr,
                                      QWindowsVistaStylePrivate::WindowTheme,
                                      WP_MINCAPTION, CS_ACTIVE, titleBarRect);
            } else
                themeData = QWindowsThemeData(widget, nullptr,
                                      QWindowsVistaStylePrivate::WindowTheme,
                                      WP_CAPTION, CS_ACTIVE, titleBarRect);
            mask->region = d->region(themeData) +
                    QRect(0, tbHeight, option->rect.width(), option->rect.height() - tbHeight);
        }
        break;
    }

#if QT_CONFIG(rubberband)
    case SH_RubberBand_Mask:
        if (qstyleoption_cast<const QStyleOptionRubberBand *>(option))
            ret = 0;
        break;
#endif // QT_CONFIG(rubberband)

    case SH_MessageBox_CenterButtons:
        ret = false;
        break;

    case SH_ToolTip_Mask:
        if (option) {
            if (QStyleHintReturnMask *mask = qstyleoption_cast<QStyleHintReturnMask*>(returnData)) {
                ret = true;
                QWindowsThemeData themeData(widget, nullptr,
                                    QWindowsVistaStylePrivate::ToolTipTheme,
                                    TTP_STANDARD, TTSS_NORMAL, option->rect);
                mask->region = d->region(themeData);
            }
        }
        break;

    case SH_Table_GridLineColor:
        if (option)
            ret = int(option->palette.color(QPalette::Base).darker(118).rgba());
        else
            ret = -1;
        break;

    case SH_Header_ArrowAlignment:
        ret = Qt::AlignTop | Qt::AlignHCenter;
        break;

    case SH_ItemView_DrawDelegateFrame:
        ret = 1;
        break;

    default:
        ret = QWindowsStyle::styleHint(hint, option, widget, returnData);
        break;
    }

    return ret;
}


/*!
 \internal

 see drawPrimitive for comments on the animation support
 */
void QWindowsVistaStyle::drawControl(ControlElement element, const QStyleOption *option,
                                     QPainter *painter, const QWidget *widget) const
{
    if (!QWindowsVistaStylePrivate::useVista()) {
        QWindowsStyle::drawControl(element, option, painter, widget);
        return;
    }

    QWindowsVistaStylePrivate *d = const_cast<QWindowsVistaStylePrivate*>(d_func());

    bool selected = option->state & State_Selected;
    bool pressed = option->state & State_Sunken;
    bool disabled = !(option->state & State_Enabled);

    int state = option->state;
    int themeNumber  = -1;

    QRect rect(option->rect);
    State flags = option->state;
    int partId = 0;
    int stateId = 0;

    if (d->transitionsEnabled() && canAnimate(option)) {
        if (element == CE_PushButtonBevel) {
            QRect oldRect;
            QRect newRect;

            QObject *styleObject = option->styleObject;

            int oldState = styleObject->property("_q_stylestate").toInt();
            oldRect = styleObject->property("_q_stylerect").toRect();
            newRect = option->rect;
            styleObject->setProperty("_q_stylestate", int(option->state));
            styleObject->setProperty("_q_stylerect", option->rect);

            bool wasDefault = false;
            bool isDefault = false;
            if (const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
                wasDefault = styleObject->property("_q_isdefault").toBool();
                isDefault = button->features & QStyleOptionButton::DefaultButton;
                styleObject->setProperty("_q_isdefault", isDefault);
            }

            bool doTransition = ((state & State_Sunken)     != (oldState & State_Sunken) ||
                    (state & State_On)         != (oldState & State_On)     ||
                    (state & State_MouseOver)  != (oldState & State_MouseOver));

            if (oldRect != newRect || (wasDefault && !isDefault)) {
                doTransition = false;
                d->stopAnimation(styleObject);
            }

            if (doTransition) {
                styleObject->setProperty("_q_no_animation", true);

                QWindowsVistaTransition *t = new QWindowsVistaTransition(styleObject);
                QWindowsVistaAnimation *anim = qobject_cast<QWindowsVistaAnimation *>(d->animation(styleObject));
                QStyleOption *styleOption = clonedAnimationStyleOption(option);
                styleOption->state = QStyle::State(oldState);

                QImage startImage = createAnimationBuffer(option, widget);
                QPainter startPainter(&startImage);

                // Use current state of existing animation if already one is running
                if (!anim) {
                    proxy()->drawControl(element, styleOption, &startPainter, widget);
                } else {
                    anim->paint(&startPainter, styleOption);
                    d->stopAnimation(styleObject);
                }

                t->setStartImage(startImage);
                QImage endImage = createAnimationBuffer(option, widget);
                QPainter endPainter(&endImage);
                styleOption->state = option->state;
                proxy()->drawControl(element, styleOption, &endPainter, widget);
                t->setEndImage(endImage);


                DWORD duration = 0;
                const HTHEME theme = OpenThemeData(nullptr, L"Button");

                int fromState = buttonStateId(oldState, BP_PUSHBUTTON);
                int toState = buttonStateId(option->state, BP_PUSHBUTTON);
                if (GetThemeTransitionDuration(theme, BP_PUSHBUTTON, fromState, toState, TMT_TRANSITIONDURATIONS, &duration) == S_OK)
                    t->setDuration(int(duration));
                else
                    t->setDuration(0);
                t->setStartTime(d->animationTime());
                styleObject->setProperty("_q_no_animation", false);

                deleteClonedAnimationStyleOption(styleOption);
                d->startAnimation(t);
            }

            QWindowsVistaAnimation *anim = qobject_cast<QWindowsVistaAnimation *>(d->animation(styleObject));
            if (anim) {
                anim->paint(painter, option);
                return;
            }

        }
    }

    bool hMirrored = false;
    bool vMirrored = false;
    int rotate = 0;

    switch (element) {
    case CE_PushButtonBevel:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option))  {
            themeNumber = QWindowsVistaStylePrivate::ButtonTheme;
            partId = BP_PUSHBUTTON;
            if (btn->features & QStyleOptionButton::CommandLinkButton)
                partId = BP_COMMANDLINK;
            bool justFlat = (btn->features & QStyleOptionButton::Flat) && !(flags & (State_On|State_Sunken));
            if (!(flags & State_Enabled) && !(btn->features & QStyleOptionButton::Flat))
                stateId = PBS_DISABLED;
            else if (justFlat)
                ;
            else if (flags & (State_Sunken | State_On))
                stateId = PBS_PRESSED;
            else if (flags & State_MouseOver)
                stateId = PBS_HOT;
            else if (btn->features & QStyleOptionButton::DefaultButton && (state & State_Active))
                stateId = PBS_DEFAULTED;
            else
                stateId = PBS_NORMAL;

            if (!justFlat) {

                if (d->transitionsEnabled() && (btn->features & QStyleOptionButton::DefaultButton) &&
                        !(state & (State_Sunken | State_On)) && !(state & State_MouseOver) &&
                        (state & State_Enabled) && (state & State_Active))
                {
                    QWindowsVistaAnimation *anim = qobject_cast<QWindowsVistaAnimation *>(d->animation(styleObject(option)));

                    if (!anim) {
                        QImage startImage = createAnimationBuffer(option, widget);
                        QImage alternateImage = createAnimationBuffer(option, widget);

                        QWindowsVistaPulse *pulse = new QWindowsVistaPulse(styleObject(option));

                        QPainter startPainter(&startImage);
                        stateId = PBS_DEFAULTED;
                        QWindowsThemeData theme(widget, &startPainter, themeNumber, partId, stateId, rect);
                        d->drawBackground(theme);

                        QPainter alternatePainter(&alternateImage);
                        theme.stateId = PBS_DEFAULTED_ANIMATING;
                        theme.painter = &alternatePainter;
                        d->drawBackground(theme);

                        pulse->setStartImage(startImage);
                        pulse->setEndImage(alternateImage);
                        pulse->setStartTime(d->animationTime());
                        pulse->setDuration(2000);
                        d->startAnimation(pulse);
                        anim = pulse;
                    }

                    if (anim)
                        anim->paint(painter, option);
                    else {
                        QWindowsThemeData theme(widget, painter, themeNumber, partId, stateId, rect);
                        d->drawBackground(theme);
                    }
                }
                else {
                    QWindowsThemeData theme(widget, painter, themeNumber, partId, stateId, rect);
                    d->drawBackground(theme);
                }
            }

            if (btn->features & QStyleOptionButton::HasMenu) {
                int mbiw = 0, mbih = 0;
                QWindowsThemeData theme(widget, nullptr, QWindowsVistaStylePrivate::ToolBarTheme,
                                TP_DROPDOWNBUTTON);
                if (theme.isValid()) {
                    const QSizeF size = theme.size() * QStyleHelper::dpiScaled(1, option);
                    if (!size.isEmpty()) {
                        mbiw = qRound(size.width());
                        mbih = qRound(size.height());
                    }
                }
                QRect ir = subElementRect(SE_PushButtonContents, option, nullptr);
                QStyleOptionButton newBtn = *btn;
                newBtn.rect = QStyle::visualRect(option->direction, option->rect,
                                                 QRect(ir.right() - mbiw - 2,
                                                       option->rect.top() + (option->rect.height()/2) - (mbih/2),
                                                       mbiw + 1, mbih + 1));
                proxy()->drawPrimitive(PE_IndicatorArrowDown, &newBtn, painter, widget);
            }
        }
        return;

    case CE_SizeGrip: {
        themeNumber = QWindowsVistaStylePrivate::StatusTheme;
        partId = SP_GRIPPER;
        QWindowsThemeData theme(nullptr, painter, themeNumber, partId);
        QSize size = (theme.size() * QWindowsStylePrivate::nativeMetricScaleFactor(widget)).toSize();
        size.rheight()--;
        if (const auto *sg = qstyleoption_cast<const QStyleOptionSizeGrip *>(option)) {
            switch (sg->corner) {
            case Qt::BottomRightCorner:
                rect = QRect(QPoint(rect.right() - size.width(), rect.bottom() - size.height()), size);
                break;
            case Qt::BottomLeftCorner:
                rect = QRect(QPoint(rect.left() + 1, rect.bottom() - size.height()), size);
                hMirrored = true;
                break;
            case Qt::TopRightCorner:
                rect = QRect(QPoint(rect.right() - size.width(), rect.top() + 1), size);
                vMirrored = true;
                break;
            case Qt::TopLeftCorner:
                rect = QRect(rect.topLeft() + QPoint(1, 1), size);
                hMirrored = vMirrored = true;
            }
        }
        break;
    }

    case CE_Splitter:
        painter->eraseRect(option->rect);
        return;

    case CE_TabBarTab:
#if QT_CONFIG(tabwidget)
        if (const auto *tab = qstyleoption_cast<const QStyleOptionTab *>(option))
            stateId = tab->state & State_Enabled ? TIS_NORMAL : TIS_DISABLED;
#endif  // QT_CONFIG(tabwidget)
        break;

    case CE_TabBarTabShape:
#if QT_CONFIG(tabwidget)
        if (const auto *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {
            themeNumber = QWindowsVistaStylePrivate::TabTheme;
            const bool isDisabled = !(tab->state & State_Enabled);
            const bool hasFocus = tab->state & State_HasFocus;
            const bool isHot = tab->state & State_MouseOver;
            const bool selected = tab->state & State_Selected;
            bool lastTab = tab->position == QStyleOptionTab::End;
            bool firstTab = tab->position == QStyleOptionTab::Beginning;
            const bool onlyOne = tab->position == QStyleOptionTab::OnlyOneTab;
            const bool leftAligned = proxy()->styleHint(SH_TabBar_Alignment, tab, widget) == Qt::AlignLeft;
            const bool centerAligned = proxy()->styleHint(SH_TabBar_Alignment, tab, widget) == Qt::AlignCenter;
            const int borderThickness = proxy()->pixelMetric(PM_DefaultFrameWidth, option, widget);
            const int tabOverlap = proxy()->pixelMetric(PM_TabBarTabOverlap, option, widget);

            if (isDisabled)
                stateId = TIS_DISABLED;
            else if (selected)
                stateId = TIS_SELECTED;
            else if (hasFocus)
                stateId = TIS_FOCUSED;
            else if (isHot)
                stateId = TIS_HOT;
            else
                stateId = TIS_NORMAL;

            // Selecting proper part depending on position
            if (firstTab || onlyOne) {
                if (leftAligned)
                    partId = TABP_TABITEMLEFTEDGE;
                else if (centerAligned)
                    partId = TABP_TABITEM;
                else // rightAligned
                    partId = TABP_TABITEMRIGHTEDGE;
            } else {
                partId = TABP_TABITEM;
            }

            if (tab->direction == Qt::RightToLeft
                    && (tab->shape == QTabBar::RoundedNorth || tab->shape == QTabBar::RoundedSouth)) {
                bool temp = firstTab;
                firstTab = lastTab;
                lastTab = temp;
            }

            const bool begin = firstTab || onlyOne;
            const bool end = lastTab || onlyOne;

            switch (tab->shape) {
            case QTabBar::RoundedNorth:
                if (selected)
                    rect.adjust(begin ? 0 : -tabOverlap, 0, end ? 0 : tabOverlap, borderThickness);
                else
                    rect.adjust(begin? tabOverlap : 0, tabOverlap, end ? -tabOverlap : 0, 0);
                break;
            case QTabBar::RoundedSouth:
                //vMirrored = true;
                rotate = 180; // Not 100% correct, but works
                if (selected)
                    rect.adjust(begin ? 0 : -tabOverlap , -borderThickness, end ? 0 : tabOverlap, 0);
                else
                    rect.adjust(begin ? tabOverlap : 0, 0, end ? -tabOverlap : 0 , -tabOverlap);
                break;
            case QTabBar::RoundedEast:
                rotate = 90;
                if (selected)
                    rect.adjust(-borderThickness, begin ? 0 : -tabOverlap, 0, end ? 0 : tabOverlap);
                else
                    rect.adjust(0, begin ? tabOverlap : 0, -tabOverlap, end ? -tabOverlap : 0);
                break;
            case QTabBar::RoundedWest:
                hMirrored = true;
                rotate = 90;
                if (selected)
                    rect.adjust(0, begin ? 0 : -tabOverlap, borderThickness, end ? 0 : tabOverlap);
                else
                    rect.adjust(tabOverlap, begin ? tabOverlap : 0, 0, end ? -tabOverlap : 0);
                break;
            default:
                themeNumber = -1; // Do our own painting for triangular
                break;
            }

            if (!selected) {
                switch (tab->shape) {
                case QTabBar::RoundedNorth:
                    rect.adjust(0,0, 0,-1);
                    break;
                case QTabBar::RoundedSouth:
                    rect.adjust(0,1, 0,0);
                    break;
                case QTabBar::RoundedEast:
                    rect.adjust( 1,0, 0,0);
                    break;
                case QTabBar::RoundedWest:
                    rect.adjust(0,0, -1,0);
                    break;
                default:
                    break;
                }
            }
        }
#endif  // QT_CONFIG(tabwidget)
        break;

    case CE_ProgressBarGroove: {
        Qt::Orientation orient = Qt::Horizontal;
        if (const auto *pb = qstyleoption_cast<const QStyleOptionProgressBar *>(option))
            if (!(pb->state & QStyle::State_Horizontal))
                orient = Qt::Vertical;

        partId = (orient == Qt::Horizontal) ? PP_BAR : PP_BARVERT;
        themeNumber = QWindowsVistaStylePrivate::ProgressTheme;
        stateId = 1;
        break;
    }

    case CE_ProgressBarContents:
        if (const auto *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            bool isIndeterminate = (bar->minimum == 0 && bar->maximum == 0);
            const bool vertical = !(bar->state & QStyle::State_Horizontal);
            const bool inverted = bar->invertedAppearance;

            if (isIndeterminate || (bar->progress > 0 && (bar->progress < bar->maximum) && d->transitionsEnabled())) {
                if (!d->animation(styleObject(option)))
                    d->startAnimation(new QProgressStyleAnimation(d->animationFps, styleObject(option)));
            } else {
                d->stopAnimation(styleObject(option));
            }

            QWindowsThemeData theme(widget, painter,
                            QWindowsVistaStylePrivate::ProgressTheme,
                            vertical ? PP_FILLVERT : PP_FILL);
            theme.rect = option->rect;
            bool reverse = (bar->direction == Qt::LeftToRight && inverted) || (bar->direction == Qt::RightToLeft && !inverted);
            QTime current = d->animationTime();

            if (isIndeterminate) {
                if (auto *progressAnimation = qobject_cast<QProgressStyleAnimation *>(d->animation(styleObject(option)))) {
                    int glowSize = 120;
                    int animationWidth = glowSize * 2 + (vertical ? theme.rect.height() : theme.rect.width());
                    int animOffset = progressAnimation->startTime().msecsTo(current) / 4;
                    if (animOffset > animationWidth)
                        progressAnimation->setStartTime(d->animationTime());
                    painter->save();
                    painter->setClipRect(theme.rect);
                    QRect animRect;
                    QSize pixmapSize(14, 14);
                    if (vertical) {
                        animRect = QRect(theme.rect.left(),
                                         inverted ? rect.top() - glowSize + animOffset :
                                                    rect.bottom() + glowSize - animOffset,
                                         rect.width(), glowSize);
                        pixmapSize.setHeight(animRect.height());
                    } else {
                        animRect = QRect(rect.left() - glowSize + animOffset,
                                         rect.top(), glowSize, rect.height());
                        animRect = QStyle::visualRect(reverse ? Qt::RightToLeft : Qt::LeftToRight,
                                                      option->rect, animRect);
                        pixmapSize.setWidth(animRect.width());
                    }
                    QString name = QStringLiteral(u"qiprogress-%1-%2").arg(pixmapSize.width()).arg(pixmapSize.height());
                    QPixmap pixmap;
                    if (!QPixmapCache::find(name, &pixmap)) {
                        QImage image(pixmapSize, QImage::Format_ARGB32);
                        image.fill(Qt::transparent);
                        QPainter imagePainter(&image);
                        theme.painter = &imagePainter;
                        theme.partId = vertical ? PP_FILLVERT : PP_FILL;
                        theme.rect = QRect(QPoint(0,0), animRect.size());
                        QLinearGradient alphaGradient(0, 0, vertical ? 0 : image.width(),
                                                      vertical ? image.height() : 0);
                        alphaGradient.setColorAt(0, QColor(0, 0, 0, 0));
                        alphaGradient.setColorAt(0.5, QColor(0, 0, 0, 220));
                        alphaGradient.setColorAt(1, QColor(0, 0, 0, 0));
                        imagePainter.fillRect(image.rect(), alphaGradient);
                        imagePainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
                        d->drawBackground(theme);
                        imagePainter.end();
                        pixmap = QPixmap::fromImage(image);
                        QPixmapCache::insert(name, pixmap);
                    }
                    painter->drawPixmap(animRect, pixmap);
                    painter->restore();
                }
            } else {
                qint64 progress = qMax<qint64>(bar->progress, bar->minimum); // workaround for bug in QProgressBar

                if (vertical) {
                    int maxHeight = option->rect.height();
                    int minHeight = 0;
                    double vc6_workaround = ((progress - qint64(bar->minimum)) / qMax(double(1.0), double(qint64(bar->maximum) - qint64(bar->minimum))) * maxHeight);
                    int height = isIndeterminate ? maxHeight: qMax(int(vc6_workaround), minHeight);
                    theme.rect.setHeight(height);
                    if (!inverted)
                        theme.rect.moveTop(rect.height() - theme.rect.height());
                } else {
                    int maxWidth = option->rect.width();
                    int minWidth = 0;
                    double vc6_workaround = ((progress - qint64(bar->minimum)) / qMax(double(1.0), double(qint64(bar->maximum) - qint64(bar->minimum))) * maxWidth);
                    int width = isIndeterminate ? maxWidth : qMax(int(vc6_workaround), minWidth);
                    theme.rect.setWidth(width);
                    theme.rect = QStyle::visualRect(reverse ? Qt::RightToLeft : Qt::LeftToRight,
                                                    option->rect, theme.rect);
                }
                d->drawBackground(theme);

                if (QProgressStyleAnimation *a = qobject_cast<QProgressStyleAnimation *>(d->animation(styleObject(option)))) {
                    int glowSize = 140;
                    int animationWidth = glowSize * 2 + (vertical ? theme.rect.height() : theme.rect.width());
                    int animOffset = a->startTime().msecsTo(current) / 4;
                    theme.partId = vertical ? PP_MOVEOVERLAYVERT : PP_MOVEOVERLAY;
                    if (animOffset > animationWidth) {
                        if (bar->progress < bar->maximum)
                            a->setStartTime(d->animationTime());
                        else
                            d->stopAnimation(styleObject(option)); //we stop the glow motion only after it has
                        //moved out of view
                    }
                    painter->save();
                    painter->setClipRect(theme.rect);
                    if (vertical) {
                        theme.rect = QRect(theme.rect.left(),
                                           inverted ? rect.top() - glowSize + animOffset :
                                                      rect.bottom() + glowSize - animOffset,
                                           rect.width(), glowSize);
                    } else {
                        theme.rect = QRect(rect.left() - glowSize + animOffset,rect.top(), glowSize, rect.height());
                        theme.rect = QStyle::visualRect(reverse ? Qt::RightToLeft : Qt::LeftToRight, option->rect, theme.rect);
                    }
                    d->drawBackground(theme);
                    painter->restore();
                }
            }
        }
        return;

    case CE_MenuBarItem:
        if (const auto *mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option))  {
            if (mbi->menuItemType == QStyleOptionMenuItem::DefaultItem)
                break;

            QPalette::ColorRole textRole = disabled ? QPalette::Text : QPalette::ButtonText;
            const auto dpr = QStyleHelper::getDpr(widget);
            const auto extent = proxy()->pixelMetric(PM_SmallIconSize, option, widget);
            const auto pix = mbi->icon.pixmap(QSize(extent, extent), dpr, QIcon::Normal);

            int alignment = Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
            if (!proxy()->styleHint(SH_UnderlineShortcut, mbi, widget))
                alignment |= Qt::TextHideMnemonic;

            if (widget && mbi->palette.color(QPalette::Window) != Qt::transparent) { // Not needed for QtQuick Controls
                //The rect adjustment is a workaround for the menu not really filling its background.
                QWindowsThemeData theme(widget, painter,
                                QWindowsVistaStylePrivate::MenuTheme,
                                MENU_BARBACKGROUND, 0, option->rect.adjusted(-1, 0, 2, 1));
                d->drawBackground(theme);

                int stateId = MBI_NORMAL;
                if (disabled)
                    stateId = MBI_DISABLED;
                else if (pressed)
                    stateId = MBI_PUSHED;
                else if (selected)
                    stateId = MBI_HOT;

                QWindowsThemeData theme2(widget, painter,
                                 QWindowsVistaStylePrivate::MenuTheme,
                                 MENU_BARITEM, stateId, option->rect);
                d->drawBackground(theme2);
            }

            if (!pix.isNull())
                drawItemPixmap(painter, mbi->rect, alignment, pix);
            else
                drawItemText(painter, mbi->rect, alignment, mbi->palette, mbi->state & State_Enabled, mbi->text, textRole);
        }
        return;

#if QT_CONFIG(menu)
    case CE_MenuEmptyArea:
        if (const auto *menuitem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            QBrush fill = menuitem->palette.brush((menuitem->state & State_Selected) ?
                                                      QPalette::Highlight : QPalette::Button);
            painter->fillRect(rect, fill);
            break;
        }
        return;

    case CE_MenuItem:
        if (const auto *menuitem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            // windows always has a check column, regardless whether we have an icon or not
            const qreal factor = QWindowsVistaStylePrivate::nativeMetricScaleFactor(widget);
            int checkcol = qRound(qreal(25) * factor);
            const int gutterWidth = qRound(qreal(3) * factor);
            {
                QWindowsThemeData theme(widget, nullptr, QWindowsVistaStylePrivate::MenuTheme,
                                MENU_POPUPCHECKBACKGROUND, MBI_HOT);
                QWindowsThemeData themeSize = theme;
                themeSize.partId = MENU_POPUPCHECK;
                themeSize.stateId = 0;
                const QSizeF size = themeSize.size() * QWindowsStylePrivate::nativeMetricScaleFactor(widget);
                const QMarginsF margins = themeSize.margins() * QWindowsStylePrivate::nativeMetricScaleFactor(widget);
                checkcol = qMax(menuitem->maxIconWidth, qRound(gutterWidth + size.width() + margins.left() + margins.right()));
            }
            QRect rect = option->rect;

            //fill popup background
            QWindowsThemeData popupbackgroundTheme(widget, painter, QWindowsVistaStylePrivate::MenuTheme,
                             MENU_POPUPBACKGROUND, stateId, option->rect);
            d->drawBackground(popupbackgroundTheme);

            //draw vertical menu line
            if (option->direction == Qt::LeftToRight)
                checkcol += rect.x();
            QPoint p1 = QStyle::visualPos(option->direction, menuitem->rect, QPoint(checkcol, rect.top()));
            QPoint p2 = QStyle::visualPos(option->direction, menuitem->rect, QPoint(checkcol, rect.bottom()));
            QRect gutterRect(p1.x(), p1.y(), gutterWidth, p2.y() - p1.y() + 1);
            QWindowsThemeData theme2(widget, painter, QWindowsVistaStylePrivate::MenuTheme,
                             MENU_POPUPGUTTER, stateId, gutterRect);
            d->drawBackground(theme2);

            int x, y, w, h;
            menuitem->rect.getRect(&x, &y, &w, &h);
            int tab = menuitem->reservedShortcutWidth;
            bool dis = !(menuitem->state & State_Enabled);
            bool checked = menuitem->checkType != QStyleOptionMenuItem::NotCheckable
                    ? menuitem->checked : false;
            bool act = menuitem->state & State_Selected;

            if (menuitem->menuItemType == QStyleOptionMenuItem::Separator) {
                int yoff = y-2 + h / 2;
                const int separatorSize = qRound(qreal(6) * QWindowsStylePrivate::nativeMetricScaleFactor(widget));
                QPoint p1 = QPoint(x + checkcol, yoff);
                QPoint p2 = QPoint(x + w + separatorSize, yoff);
                stateId = MBI_HOT;
                QRect subRect(p1.x() + (gutterWidth - menuitem->rect.x()), p1.y(),
                              p2.x() - p1.x(), separatorSize);
                subRect  = QStyle::visualRect(option->direction, option->rect, subRect );
                QWindowsThemeData theme2(widget, painter,
                                 QWindowsVistaStylePrivate::MenuTheme,
                                 MENU_POPUPSEPARATOR, stateId, subRect);
                d->drawBackground(theme2);
                return;
            }

            QRect vCheckRect = visualRect(option->direction, menuitem->rect, QRect(menuitem->rect.x(),
                                                                                   menuitem->rect.y(), checkcol - (gutterWidth + menuitem->rect.x()), menuitem->rect.height()));

            if (act) {
                stateId = dis ? MBI_DISABLED : MBI_HOT;
                QWindowsThemeData theme2(widget, painter,
                                 QWindowsVistaStylePrivate::MenuTheme,
                                 MENU_POPUPITEM, stateId, option->rect);
                d->drawBackground(theme2);
            }

            if (checked) {
                QWindowsThemeData theme(widget, painter,
                                QWindowsVistaStylePrivate::MenuTheme,
                                MENU_POPUPCHECKBACKGROUND,
                                menuitem->icon.isNull() ? MBI_HOT : MBI_PUSHED, vCheckRect);
                QWindowsThemeData themeSize = theme;
                themeSize.partId = MENU_POPUPCHECK;
                themeSize.stateId = 0;
                const QSizeF size = themeSize.size() * QWindowsStylePrivate::nativeMetricScaleFactor(widget);
                const QMarginsF margins = themeSize.margins() * QWindowsStylePrivate::nativeMetricScaleFactor(widget);
                QRect checkRect(0, 0, qRound(size.width() + margins.left() + margins.right()),
                                qRound(size.height() + margins.bottom() + margins.top()));
                checkRect.moveCenter(vCheckRect.center());
                theme.rect = checkRect;

                d->drawBackground(theme);

                if (menuitem->icon.isNull()) {
                    checkRect = QRect(QPoint(0, 0), size.toSize());
                    checkRect.moveCenter(theme.rect.center());
                    theme.rect = checkRect;

                    theme.partId = MENU_POPUPCHECK;
                    bool bullet = menuitem->checkType & QStyleOptionMenuItem::Exclusive;
                    if (dis)
                        theme.stateId = bullet ? MC_BULLETDISABLED: MC_CHECKMARKDISABLED;
                    else
                        theme.stateId = bullet ? MC_BULLETNORMAL: MC_CHECKMARKNORMAL;
                    d->drawBackground(theme);
                }
            }

            if (!menuitem->icon.isNull()) {
                QIcon::Mode mode = dis ? QIcon::Disabled : QIcon::Normal;
                if (act && !dis)
                    mode = QIcon::Active;
                const auto size = proxy()->pixelMetric(PM_SmallIconSize, option, widget);
                QRect pmr(QPoint(0, 0), QSize(size, size));
                pmr.moveCenter(vCheckRect.center());
                menuitem->icon.paint(painter, vCheckRect, Qt::AlignCenter, mode,
                                     checked ? QIcon::On : QIcon::Off);
            }

            painter->setPen(menuitem->palette.buttonText().color());

            const QColor textColor = menuitem->palette.text().color();
            if (dis)
                painter->setPen(textColor);

            int xm = windowsItemFrame + checkcol + windowsItemHMargin + (gutterWidth - menuitem->rect.x()) - 1;
            int xpos = menuitem->rect.x() + xm;
            QRect textRect(xpos, y + windowsItemVMargin, w - xm - windowsRightBorder - tab + 1, h - 2 * windowsItemVMargin);
            QRect vTextRect = visualRect(option->direction, menuitem->rect, textRect);
            QString s = menuitem->text;
            if (!s.isEmpty()) {    // draw text
                painter->save();
                int t = s.indexOf(QLatin1Char('\t'));
                int text_flags = Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                if (!proxy()->styleHint(SH_UnderlineShortcut, menuitem, widget))
                    text_flags |= Qt::TextHideMnemonic;
                text_flags |= Qt::AlignLeft;
                if (t >= 0) {
                    QRect vShortcutRect = visualRect(option->direction, menuitem->rect,
                                                     QRect(textRect.topRight(), QPoint(menuitem->rect.right(), textRect.bottom())));
                    painter->drawText(vShortcutRect, text_flags, s.mid(t + 1));
                    s = s.left(t);
                }
                QFont font = menuitem->font;
                if (menuitem->menuItemType == QStyleOptionMenuItem::DefaultItem)
                    font.setBold(true);
                painter->setFont(font);
                painter->setPen(textColor);
                painter->drawText(vTextRect, text_flags, s.left(t));
                painter->restore();
            }
            if (menuitem->menuItemType == QStyleOptionMenuItem::SubMenu) {// draw sub menu arrow
                int dim = (h - 2 * windowsItemFrame) / 2;
                PrimitiveElement arrow;
                arrow = (option->direction == Qt::RightToLeft) ? PE_IndicatorArrowLeft : PE_IndicatorArrowRight;
                xpos = x + w - windowsArrowHMargin - windowsItemFrame - dim;
                QRect  vSubMenuRect = visualRect(option->direction, menuitem->rect, QRect(xpos, y + h / 2 - dim / 2, dim, dim));
                QStyleOptionMenuItem newMI = *menuitem;
                newMI.rect = vSubMenuRect;
                newMI.state = dis ? State_None : State_Enabled;
                proxy()->drawPrimitive(arrow, &newMI, painter, widget);
            }
        }
        return;
#endif // QT_CONFIG(menu)

    case CE_HeaderSection:
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            partId = HP_HEADERITEM;
            if (flags & State_Sunken)
                stateId = HIS_PRESSED;
            else if (flags & State_MouseOver)
                stateId = HIS_HOT;
            else
                stateId = HIS_NORMAL;

            if (header->sortIndicator != QStyleOptionHeader::None)
                stateId += 3;

            QWindowsThemeData theme(widget, painter,
                            QWindowsVistaStylePrivate::HeaderTheme,
                            partId, stateId, option->rect);
            d->drawBackground(theme);
        }
        return;

    case CE_MenuBarEmptyArea: {
        stateId = MBI_NORMAL;
        if (!(state & State_Enabled))
            stateId = MBI_DISABLED;
        QWindowsThemeData theme(widget, painter, QWindowsVistaStylePrivate::MenuTheme,
                        MENU_BARBACKGROUND, stateId, option->rect);
        d->drawBackground(theme);
        return;
    }

    case CE_ToolBar:
#if QT_CONFIG(toolbar)
        if (const auto *toolbar = qstyleoption_cast<const QStyleOptionToolBar *>(option)) {
            QPalette pal = option->palette;
            pal.setColor(QPalette::Dark, option->palette.window().color().darker(130));
            QStyleOptionToolBar copyOpt = *toolbar;
            copyOpt.palette = pal;
            QWindowsStyle::drawControl(element, &copyOpt, painter, widget);
        }
#endif  // QT_CONFIG(toolbar)
        return;

#if QT_CONFIG(dockwidget)
    case CE_DockWidgetTitle:
        if (const auto *dwOpt = qstyleoption_cast<const QStyleOptionDockWidget *>(option)) {
            QRect rect = option->rect;
            const QDockWidget *dw = qobject_cast<const QDockWidget *>(widget);
            bool isFloating = dw && dw->isFloating();
            int buttonMargin = 4;
            int mw = proxy()->pixelMetric(QStyle::PM_DockWidgetTitleMargin, dwOpt, widget);
            int fw = proxy()->pixelMetric(PM_DockWidgetFrameWidth, dwOpt, widget);

            const bool verticalTitleBar = dwOpt->verticalTitleBar;

            if (verticalTitleBar) {
                rect = rect.transposed();

                painter->translate(rect.left() - 1, rect.top() + rect.width());
                painter->rotate(-90);
                painter->translate(-rect.left() + 1, -rect.top());
            }

            QRect r = option->rect.adjusted(0, 2, -1, -3);
            QRect titleRect = r;

            if (dwOpt->closable) {
                QSize sz = proxy()->standardIcon(QStyle::SP_TitleBarCloseButton, dwOpt, widget).actualSize(QSize(10, 10));
                titleRect.adjust(0, 0, -sz.width() - mw - buttonMargin, 0);
            }

            if (dwOpt->floatable) {
                QSize sz = proxy()->standardIcon(QStyle::SP_TitleBarMaxButton, dwOpt, widget).actualSize(QSize(10, 10));
                titleRect.adjust(0, 0, -sz.width() - mw - buttonMargin, 0);
            }

            if (isFloating) {
                titleRect.adjust(0, -fw, 0, 0);
                if (widget && widget->windowIcon().cacheKey() != QApplication::windowIcon().cacheKey())
                    titleRect.adjust(titleRect.height() + mw, 0, 0, 0);
            } else {
                titleRect.adjust(mw, 0, 0, 0);
                if (!dwOpt->floatable && !dwOpt->closable)
                    titleRect.adjust(0, 0, -mw, 0);
            }

            if (!verticalTitleBar)
                titleRect = visualRect(dwOpt->direction, r, titleRect);

            if (isFloating) {
                const bool isActive = dwOpt->state & State_Active;
                themeNumber = QWindowsVistaStylePrivate::WindowTheme;
                if (isActive)
                    stateId = CS_ACTIVE;
                else
                    stateId = CS_INACTIVE;

                rect = rect.adjusted(-fw, -fw, fw, 0);

                QWindowsThemeData theme(widget, painter, themeNumber, 0, stateId);
                if (!theme.isValid())
                    break;

                // Draw small type title bar
                theme.rect = rect;
                theme.partId = WP_SMALLCAPTION;
                d->drawBackground(theme);

                // Figure out maximal button space on title bar

                QIcon ico = widget->windowIcon();
                bool hasIcon = (ico.cacheKey() != QApplication::windowIcon().cacheKey());
                if (hasIcon) {
                    const auto titleHeight = rect.height() - 2;
                    const auto dpr = QStyleHelper::getDpr(widget);
                    const auto pxIco = ico.pixmap(QSize(titleHeight, titleHeight), dpr);
                    if (!verticalTitleBar && dwOpt->direction == Qt::RightToLeft)
                        painter->drawPixmap(rect.width() - titleHeight - pxIco.width(), rect.bottom() - titleHeight - 2, pxIco);
                    else
                        painter->drawPixmap(fw, rect.bottom() - titleHeight - 2, pxIco);
                }
                if (!dwOpt->title.isEmpty()) {
                    QPen oldPen = painter->pen();
                    QFont oldFont = painter->font();
                    QFont titleFont = oldFont;
                    titleFont.setBold(true);
                    painter->setFont(titleFont);
                    QString titleText
                            = painter->fontMetrics().elidedText(dwOpt->title, Qt::ElideRight, titleRect.width());

                    int result = TST_NONE;
                    GetThemeEnumValue(theme.handle(), WP_SMALLCAPTION, isActive ? CS_ACTIVE : CS_INACTIVE, TMT_TEXTSHADOWTYPE, &result);
                    if (result != TST_NONE) {
                        COLORREF textShadowRef;
                        GetThemeColor(theme.handle(), WP_SMALLCAPTION, isActive ? CS_ACTIVE : CS_INACTIVE, TMT_TEXTSHADOWCOLOR, &textShadowRef);
                        QColor textShadow = qRgb(GetRValue(textShadowRef), GetGValue(textShadowRef), GetBValue(textShadowRef));
                        painter->setPen(textShadow);
                        drawItemText(painter, titleRect.adjusted(1, 1, 1, 1),
                                     Qt::AlignLeft | Qt::AlignBottom | Qt::TextHideMnemonic, dwOpt->palette,
                                     dwOpt->state & State_Enabled, titleText);
                    }

                    COLORREF captionText = GetSysColor(isActive ? COLOR_CAPTIONTEXT : COLOR_INACTIVECAPTIONTEXT);
                    QColor textColor = qRgb(GetRValue(captionText), GetGValue(captionText), GetBValue(captionText));
                    painter->setPen(textColor);
                    drawItemText(painter, titleRect,
                                 Qt::AlignLeft | Qt::AlignBottom | Qt::TextHideMnemonic, dwOpt->palette,
                                 dwOpt->state & State_Enabled, titleText);
                    painter->setFont(oldFont);
                    painter->setPen(oldPen);
                }
            }  else {
                painter->setBrush(option->palette.window().color().darker(110));
                painter->setPen(option->palette.window().color().darker(130));
                painter->drawRect(rect.adjusted(0, 1, -1, -3));

                if (!dwOpt->title.isEmpty()) {
                    QString titleText = painter->fontMetrics().elidedText(dwOpt->title, Qt::ElideRight,
                                                                          verticalTitleBar ? titleRect.height() : titleRect.width());
                    const int indent = 4;
                    drawItemText(painter, rect.adjusted(indent + 1, 1, -indent - 1, -1),
                                 Qt::AlignLeft | Qt::AlignVCenter | Qt::TextHideMnemonic,
                                 dwOpt->palette,
                                 dwOpt->state & State_Enabled, titleText,
                                 QPalette::WindowText);
                }
            }
        }
        return;
#endif // QT_CONFIG(dockwidget)

#if QT_CONFIG(rubberband)
    case CE_RubberBand:
        if (qstyleoption_cast<const QStyleOptionRubberBand *>(option)) {
            QColor highlight = option->palette.color(QPalette::Active, QPalette::Highlight);
            painter->save();
            painter->setPen(highlight.darker(120));
            QColor dimHighlight(qMin(highlight.red()/2 + 110, 255),
                                qMin(highlight.green()/2 + 110, 255),
                                qMin(highlight.blue()/2 + 110, 255),
                                (widget && widget->isWindow())? 255 : 127);
            painter->setBrush(dimHighlight);
            painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
            painter->restore();
            return;
        }
        break;
#endif // QT_CONFIG(rubberband)

    case CE_HeaderEmptyArea:
        if (option->state & State_Horizontal) {
            themeNumber = QWindowsVistaStylePrivate::HeaderTheme;
            stateId = HIS_NORMAL;
        } else {
            QWindowsStyle::drawControl(CE_HeaderEmptyArea, option, painter, widget);
            return;
        }
        break;

#if QT_CONFIG(itemviews)
    case CE_ItemViewItem: {
        const QStyleOptionViewItem *vopt;
        const QAbstractItemView *view = qobject_cast<const QAbstractItemView *>(widget);
        bool newStyle = true;

        if (qobject_cast<const QTableView*>(widget))
            newStyle = false;

        QWindowsThemeData theme(widget, painter, themeNumber, partId, stateId, rect);

        if (newStyle && view && (vopt = qstyleoption_cast<const QStyleOptionViewItem *>(option))) {
            /*
                    // We cannot currently get the correct selection color for "explorer style" views
                    COLORREF cref = 0;
                    QWindowsThemeData theme(d->treeViewHelper(), 0, QLatin1String("LISTVIEW"), 0, 0);
                    unsigned int res = GetThemeColor(theme.handle(), LVP_LISTITEM, LISS_SELECTED, TMT_TEXTCOLOR, &cref);
                    QColor textColor(GetRValue(cref), GetGValue(cref), GetBValue(cref));
                    */
            QPalette palette = vopt->palette;
            palette.setColor(QPalette::All, QPalette::HighlightedText, palette.color(QPalette::Active, QPalette::Text));
            // Note that setting a saturated color here results in ugly XOR colors in the focus rect
            palette.setColor(QPalette::All, QPalette::Highlight, palette.base().color().darker(108));
            QStyleOptionViewItem adjustedOption = *vopt;
            adjustedOption.palette = palette;
            // We hide the  focusrect in singleselection as it is not required
            if ((view->selectionMode() == QAbstractItemView::SingleSelection)
                    && !(vopt->state & State_KeyboardFocusChange))
                adjustedOption.state &= ~State_HasFocus;
            if (!theme.isValid()) {
                QWindowsStyle::drawControl(element, &adjustedOption, painter, widget);
                return;
            }
        } else {
            if (!theme.isValid()) {
                QWindowsStyle::drawControl(element, option, painter, widget);
                return;
            }
        }

        theme.rotate = rotate;
        theme.mirrorHorizontally = hMirrored;
        theme.mirrorVertically = vMirrored;
        d->drawBackground(theme);
        return;
    }
#endif // QT_CONFIG(itemviews)

#if QT_CONFIG(combobox)
    case CE_ComboBoxLabel:
        QCommonStyle::drawControl(element, option, painter, widget);
        return;
#endif // QT_CONFIG(combobox)

    default:
        break;
    }

    QWindowsThemeData theme(widget, painter, themeNumber, partId, stateId, rect);

    if (!theme.isValid()) {
        QWindowsStyle::drawControl(element, option, painter, widget);
        return;
    }

    theme.rotate = rotate;
    theme.mirrorHorizontally = hMirrored;
    theme.mirrorVertically = vMirrored;

    d->drawBackground(theme);
}

/*!
  \internal
  see drawPrimitive for comments on the animation support

 */
void QWindowsVistaStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                            QPainter *painter, const QWidget *widget) const
{
    QWindowsVistaStylePrivate *d = const_cast<QWindowsVistaStylePrivate*>(d_func());

    if (!QWindowsVistaStylePrivate::useVista()) {
        QWindowsStyle::drawComplexControl(control, option, painter, widget);
        return;
    }

    State state = option->state;
    SubControls sub = option->subControls;
    QRect r = option->rect;

    int partId = 0;
    int stateId = 0;

    State flags = option->state;
    if (widget && widget->testAttribute(Qt::WA_UnderMouse) && widget->isActiveWindow())
        flags |= State_MouseOver;

    if (d->transitionsEnabled() && canAnimate(option))
    {
        if (control == CC_ScrollBar || control == CC_SpinBox || control == CC_ComboBox) {
            QObject *styleObject = option->styleObject; // Can be widget or qquickitem

            int oldState = styleObject->property("_q_stylestate").toInt();
            int oldActiveControls = styleObject->property("_q_stylecontrols").toInt();

            QRect oldRect = styleObject->property("_q_stylerect").toRect();
            styleObject->setProperty("_q_stylestate", int(option->state));
            styleObject->setProperty("_q_stylecontrols", int(option->activeSubControls));
            styleObject->setProperty("_q_stylerect", option->rect);

            bool doTransition = ((state & State_Sunken) != (oldState & State_Sunken)
                    || (state & State_On) != (oldState & State_On)
                    || (state & State_MouseOver) != (oldState & State_MouseOver)
                    || oldActiveControls != int(option->activeSubControls));

            if (qstyleoption_cast<const QStyleOptionSlider *>(option)) {
                QRect oldSliderPos = styleObject->property("_q_stylesliderpos").toRect();
                QRect currentPos = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSlider, widget);
                styleObject->setProperty("_q_stylesliderpos", currentPos);
                if (oldSliderPos != currentPos) {
                    doTransition = false;
                    d->stopAnimation(styleObject);
                }
            } else if (control == CC_SpinBox) {
                //spinboxes have a transition when focus changes
                if (!doTransition)
                    doTransition = (state & State_HasFocus) != (oldState & State_HasFocus);
            }

            if (oldRect != option->rect) {
                doTransition = false;
                d->stopAnimation(styleObject);
            }

            if (doTransition) {
                QImage startImage = createAnimationBuffer(option, widget);
                QPainter startPainter(&startImage);

                QImage endImage = createAnimationBuffer(option, widget);
                QPainter endPainter(&endImage);

                QWindowsVistaAnimation *anim = qobject_cast<QWindowsVistaAnimation *>(d->animation(styleObject));
                QWindowsVistaTransition *t = new QWindowsVistaTransition(styleObject);

                // Draw the image that ends the animation by using the current styleoption
                QStyleOptionComplex *styleOption = qstyleoption_cast<QStyleOptionComplex*>(clonedAnimationStyleOption(option));

                styleObject->setProperty("_q_no_animation", true);

                // Draw transition source
                if (!anim) {
                    styleOption->state = QStyle::State(oldState);
                    styleOption->activeSubControls = QStyle::SubControl(oldActiveControls);
                    proxy()->drawComplexControl(control, styleOption, &startPainter, widget);
                } else {
                    anim->paint(&startPainter, option);
                }
                t->setStartImage(startImage);

                // Draw transition target
                styleOption->state = option->state;
                styleOption->activeSubControls = option->activeSubControls;
                proxy()->drawComplexControl(control, styleOption, &endPainter, widget);

                styleObject->setProperty("_q_no_animation", false);

                t->setEndImage(endImage);
                t->setStartTime(d->animationTime());

                if (option->state & State_MouseOver || option->state & State_Sunken)
                    t->setDuration(150);
                else
                    t->setDuration(500);

                deleteClonedAnimationStyleOption(styleOption);
                d->startAnimation(t);
            }
            if (QWindowsVistaAnimation *anim = qobject_cast<QWindowsVistaAnimation *>(d->animation(styleObject))) {
                anim->paint(painter, option);
                return;
            }
        }
    }

    switch (control) {

#if QT_CONFIG(slider)
    case CC_Slider:
        if (const auto *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            QWindowsThemeData theme(widget, painter, QWindowsVistaStylePrivate::TrackBarTheme);
            QRect slrect = slider->rect;
            QRegion tickreg = slrect;
            if (sub & SC_SliderGroove) {
                theme.rect = proxy()->subControlRect(CC_Slider, option, SC_SliderGroove, widget);
                if (slider->orientation == Qt::Horizontal) {
                    partId = TKP_TRACK;
                    stateId = TRS_NORMAL;
                    theme.rect = QRect(slrect.left(), theme.rect.center().y() - 2, slrect.width(), 4);
                } else {
                    partId = TKP_TRACKVERT;
                    stateId = TRVS_NORMAL;
                    theme.rect = QRect(theme.rect.center().x() - 2, slrect.top(), 4, slrect.height());
                }
                theme.partId = partId;
                theme.stateId = stateId;
                d->drawBackground(theme);
                tickreg -= theme.rect;
            }
            if (sub & SC_SliderTickmarks) {
                int tickOffset = proxy()->pixelMetric(PM_SliderTickmarkOffset, slider, widget);
                int ticks = slider->tickPosition;
                int thickness = proxy()->pixelMetric(PM_SliderControlThickness, slider, widget);
                int len = proxy()->pixelMetric(PM_SliderLength, slider, widget);
                int available = proxy()->pixelMetric(PM_SliderSpaceAvailable, slider, widget);
                int interval = slider->tickInterval;
                if (interval <= 0) {
                    interval = slider->singleStep;
                    if (QStyle::sliderPositionFromValue(slider->minimum, slider->maximum, interval,
                                                        available)
                            - QStyle::sliderPositionFromValue(slider->minimum, slider->maximum,
                                                              0, available) < 3)
                        interval = slider->pageStep;
                }
                if (!interval)
                    interval = 1;
                int fudge = len / 2;
                int pos;
                int bothOffset = (ticks & QSlider::TicksAbove && ticks & QSlider::TicksBelow) ? 1 : 0;
                painter->setPen(d->sliderTickColor);
                QVarLengthArray<QLine, 32> lines;
                int v = slider->minimum;
                while (v <= slider->maximum + 1) {
                    if (v == slider->maximum + 1 && interval == 1)
                        break;
                    const int v_ = qMin(v, slider->maximum);
                    int tickLength = (v_ == slider->minimum || v_ >= slider->maximum) ? 4 : 3;
                    pos = QStyle::sliderPositionFromValue(slider->minimum, slider->maximum,
                                                          v_, available) + fudge;
                    if (slider->orientation == Qt::Horizontal) {
                        if (ticks & QSlider::TicksAbove) {
                            lines.append(QLine(pos, tickOffset - 1 - bothOffset,
                                               pos, tickOffset - 1 - bothOffset - tickLength));
                        }

                        if (ticks & QSlider::TicksBelow) {
                            lines.append(QLine(pos, tickOffset + thickness + bothOffset,
                                               pos, tickOffset + thickness + bothOffset + tickLength));
                        }
                    } else {
                        if (ticks & QSlider::TicksAbove) {
                            lines.append(QLine(tickOffset - 1 - bothOffset, pos,
                                               tickOffset - 1 - bothOffset - tickLength, pos));
                        }

                        if (ticks & QSlider::TicksBelow) {
                            lines.append(QLine(tickOffset + thickness + bothOffset, pos,
                                               tickOffset + thickness + bothOffset + tickLength, pos));
                        }
                    }
                    // in the case where maximum is max int
                    int nextInterval = v + interval;
                    if (nextInterval < v)
                        break;
                    v = nextInterval;
                }
                if (!lines.isEmpty()) {
                    painter->save();
                    painter->translate(slrect.topLeft());
                    painter->drawLines(lines.constData(), lines.size());
                    painter->restore();
                }
            }
            if (sub & SC_SliderHandle) {
                theme.rect = proxy()->subControlRect(CC_Slider, option, SC_SliderHandle, widget);
                if (slider->orientation == Qt::Horizontal) {
                    if (slider->tickPosition == QSlider::TicksAbove)
                        partId = TKP_THUMBTOP;
                    else if (slider->tickPosition == QSlider::TicksBelow)
                        partId = TKP_THUMBBOTTOM;
                    else
                        partId = TKP_THUMB;

                    if (!(slider->state & State_Enabled))
                        stateId = TUS_DISABLED;
                    else if (slider->activeSubControls & SC_SliderHandle && (slider->state & State_Sunken))
                        stateId = TUS_PRESSED;
                    else if (slider->activeSubControls & SC_SliderHandle && (slider->state & State_MouseOver))
                        stateId = TUS_HOT;
                    else if (flags & State_HasFocus)
                        stateId = TUS_FOCUSED;
                    else
                        stateId = TUS_NORMAL;
                } else {
                    if (slider->tickPosition == QSlider::TicksLeft)
                        partId = TKP_THUMBLEFT;
                    else if (slider->tickPosition == QSlider::TicksRight)
                        partId = TKP_THUMBRIGHT;
                    else
                        partId = TKP_THUMBVERT;

                    if (!(slider->state & State_Enabled))
                        stateId = TUVS_DISABLED;
                    else if (slider->activeSubControls & SC_SliderHandle && (slider->state & State_Sunken))
                        stateId = TUVS_PRESSED;
                    else if (slider->activeSubControls & SC_SliderHandle && (slider->state & State_MouseOver))
                        stateId = TUVS_HOT;
                    else if (flags & State_HasFocus)
                        stateId = TUVS_FOCUSED;
                    else
                        stateId = TUVS_NORMAL;
                }
                theme.partId = partId;
                theme.stateId = stateId;
                d->drawBackground(theme);
            }
            if (slider->state & State_HasFocus) {
                QStyleOptionFocusRect fropt;
                fropt.QStyleOption::operator=(*slider);
                fropt.rect = subElementRect(SE_SliderFocusRect, slider, widget);
                proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
            }
        }
        break;
#endif

#if QT_CONFIG(toolbutton)
    case CC_ToolButton:
        if (const auto *toolbutton = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            QRect button, menuarea;
            button = proxy()->subControlRect(control, toolbutton, SC_ToolButton, widget);
            menuarea = proxy()->subControlRect(control, toolbutton, SC_ToolButtonMenu, widget);

            State bflags = toolbutton->state & ~State_Sunken;
            State mflags = bflags;
            bool autoRaise = flags & State_AutoRaise;
            if (autoRaise) {
                if (!(bflags & State_MouseOver) || !(bflags & State_Enabled))
                    bflags &= ~State_Raised;
            }

            if (toolbutton->state & State_Sunken) {
                if (toolbutton->activeSubControls & SC_ToolButton) {
                    bflags |= State_Sunken;
                    mflags |= State_MouseOver | State_Sunken;
                } else if (toolbutton->activeSubControls & SC_ToolButtonMenu) {
                    mflags |= State_Sunken;
                    bflags |= State_MouseOver;
                }
            }

            QStyleOption tool = *toolbutton;
            if (toolbutton->subControls & SC_ToolButton) {
                if (flags & (State_Sunken | State_On | State_Raised) || !autoRaise) {
                    if (toolbutton->features & QStyleOptionToolButton::MenuButtonPopup && autoRaise) {
                        QWindowsThemeData theme(widget, painter, QWindowsVistaStylePrivate::ToolBarTheme);
                        theme.partId = TP_SPLITBUTTON;
                        theme.rect = button;
                        if (!(bflags & State_Enabled))
                            stateId = TS_DISABLED;
                        else if (bflags & State_Sunken)
                            stateId = TS_PRESSED;
                        else if (bflags & State_MouseOver || !(flags & State_AutoRaise))
                            stateId = flags & State_On ? TS_HOTCHECKED : TS_HOT;
                        else if (bflags & State_On)
                            stateId = TS_CHECKED;
                        else
                            stateId = TS_NORMAL;
                        if (option->direction == Qt::RightToLeft)
                            theme.mirrorHorizontally = true;
                        theme.stateId = stateId;
                        d->drawBackground(theme);
                    } else {
                        tool.rect = option->rect;
                        tool.state = bflags;
                        if (autoRaise) // for tool bars
                            proxy()->drawPrimitive(PE_PanelButtonTool, &tool, painter, widget);
                        else
                            proxy()->drawPrimitive(PE_PanelButtonBevel, &tool, painter, widget);
                    }
                }
            }

            if (toolbutton->state & State_HasFocus) {
                QStyleOptionFocusRect fr;
                fr.QStyleOption::operator=(*toolbutton);
                fr.rect.adjust(3, 3, -3, -3);
                if (toolbutton->features & QStyleOptionToolButton::MenuButtonPopup)
                    fr.rect.adjust(0, 0, -proxy()->pixelMetric(QStyle::PM_MenuButtonIndicator,
                                                               toolbutton, widget), 0);
                proxy()->drawPrimitive(PE_FrameFocusRect, &fr, painter, widget);
            }
            QStyleOptionToolButton label = *toolbutton;
            label.state = bflags;
            int fw = 2;
            if (!autoRaise)
                label.state &= ~State_Sunken;
            label.rect = button.adjusted(fw, fw, -fw, -fw);
            proxy()->drawControl(CE_ToolButtonLabel, &label, painter, widget);

            if (toolbutton->subControls & SC_ToolButtonMenu) {
                tool.rect = menuarea;
                tool.state = mflags;
                if (autoRaise) {
                    proxy()->drawPrimitive(PE_IndicatorButtonDropDown, &tool, painter, widget);
                } else {
                    tool.state = mflags;
                    menuarea.adjust(-2, 0, 0, 0);
                    // Draw menu button
                    if ((bflags & State_Sunken) != (mflags & State_Sunken)){
                        painter->save();
                        painter->setClipRect(menuarea);
                        tool.rect = option->rect;
                        proxy()->drawPrimitive(PE_PanelButtonBevel, &tool, painter, nullptr);
                        painter->restore();
                    }
                    // Draw arrow
                    painter->save();
                    painter->setPen(option->palette.dark().color());
                    painter->drawLine(menuarea.left(), menuarea.top() + 3,
                                      menuarea.left(), menuarea.bottom() - 3);
                    painter->setPen(option->palette.light().color());
                    painter->drawLine(menuarea.left() - 1, menuarea.top() + 3,
                                      menuarea.left() - 1, menuarea.bottom() - 3);

                    tool.rect = menuarea.adjusted(2, 3, -2, -1);
                    proxy()->drawPrimitive(PE_IndicatorArrowDown, &tool, painter, widget);
                    painter->restore();
                }
            } else if (toolbutton->features & QStyleOptionToolButton::HasMenu) {
                int mbi = proxy()->pixelMetric(PM_MenuButtonIndicator, toolbutton, widget);
                QRect ir = toolbutton->rect;
                QStyleOptionToolButton newBtn = *toolbutton;
                newBtn.rect = QRect(ir.right() + 4 - mbi, ir.height() - mbi + 4, mbi - 5, mbi - 5);
                proxy()->drawPrimitive(PE_IndicatorArrowDown, &newBtn, painter, widget);
            }
        }
        break;
#endif // QT_CONFIG(toolbutton)

    case CC_TitleBar:
        if (const auto *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(option)) {
            const qreal factor = QWindowsStylePrivate::nativeMetricScaleFactor(widget);
            bool isActive = tb->titleBarState & QStyle::State_Active;
            QWindowsThemeData theme(widget, painter, QWindowsVistaStylePrivate::WindowTheme);
            if (sub & SC_TitleBarLabel) {
                partId = (tb->titleBarState & Qt::WindowMinimized) ? WP_MINCAPTION : WP_CAPTION;
                theme.rect = option->rect;
                if (widget && !widget->isEnabled())
                    stateId = CS_DISABLED;
                else if (isActive)
                    stateId = CS_ACTIVE;
                else
                    stateId = CS_INACTIVE;

                theme.partId = partId;
                theme.stateId = stateId;
                d->drawBackground(theme);

                QRect ir = proxy()->subControlRect(CC_TitleBar, tb, SC_TitleBarLabel, widget);

                int result = TST_NONE;
                GetThemeEnumValue(theme.handle(), WP_CAPTION, isActive ? CS_ACTIVE : CS_INACTIVE, TMT_TEXTSHADOWTYPE, &result);
                if (result != TST_NONE) {
                    COLORREF textShadowRef;
                    GetThemeColor(theme.handle(), WP_CAPTION, isActive ? CS_ACTIVE : CS_INACTIVE, TMT_TEXTSHADOWCOLOR, &textShadowRef);
                    QColor textShadow = qRgb(GetRValue(textShadowRef), GetGValue(textShadowRef), GetBValue(textShadowRef));
                    painter->setPen(textShadow);
                    painter->drawText(int(ir.x() + 3 * factor), int(ir.y() + 2 * factor),
                                      int(ir.width() - 1 * factor), ir.height(),
                                      Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, tb->text);
                }
                COLORREF captionText = GetSysColor(isActive ? COLOR_CAPTIONTEXT : COLOR_INACTIVECAPTIONTEXT);
                QColor textColor = qRgb(GetRValue(captionText), GetGValue(captionText), GetBValue(captionText));
                painter->setPen(textColor);
                painter->drawText(int(ir.x() + 2 * factor), int(ir.y() + 1 * factor),
                                  int(ir.width() - 2 * factor), ir.height(),
                                  Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, tb->text);
            }
            if (sub & SC_TitleBarSysMenu && tb->titleBarFlags & Qt::WindowSystemMenuHint) {
                theme.rect = proxy()->subControlRect(CC_TitleBar, option, SC_TitleBarSysMenu, widget);
                partId = WP_SYSBUTTON;
                if ((widget && !widget->isEnabled()) || !isActive)
                    stateId = SBS_DISABLED;
                else if (option->activeSubControls == SC_TitleBarSysMenu && (option->state & State_Sunken))
                    stateId = SBS_PUSHED;
                else if (option->activeSubControls == SC_TitleBarSysMenu && (option->state & State_MouseOver))
                    stateId = SBS_HOT;
                else
                    stateId = SBS_NORMAL;
                if (!tb->icon.isNull()) {
                    tb->icon.paint(painter, theme.rect);
                } else {
                    theme.partId = partId;
                    theme.stateId = stateId;
                    if (theme.size().isEmpty()) {
                        const auto extent = proxy()->pixelMetric(PM_SmallIconSize, tb, widget);
                        const auto dpr = QStyleHelper::getDpr(widget);
                        const auto icon = proxy()->standardIcon(SP_TitleBarMenuButton, tb, widget);
                        const auto pm = icon.pixmap(QSize(extent, extent), dpr);
                        drawItemPixmap(painter, theme.rect, Qt::AlignCenter, pm);
                    } else {
                        d->drawBackground(theme);
                    }
                }
            }

            if (sub & SC_TitleBarMinButton && tb->titleBarFlags & Qt::WindowMinimizeButtonHint
                    && !(tb->titleBarState & Qt::WindowMinimized)) {
                populateTitleBarButtonTheme(proxy(), widget, option, SC_TitleBarMinButton, isActive, WP_MINBUTTON, &theme);
                d->drawBackground(theme);
            }
            if (sub & SC_TitleBarMaxButton && tb->titleBarFlags & Qt::WindowMaximizeButtonHint
                    && !(tb->titleBarState & Qt::WindowMaximized)) {
                populateTitleBarButtonTheme(proxy(), widget, option, SC_TitleBarMaxButton, isActive, WP_MAXBUTTON, &theme);
                d->drawBackground(theme);
            }
            if (sub & SC_TitleBarContextHelpButton
                    && tb->titleBarFlags & Qt::WindowContextHelpButtonHint) {
                populateTitleBarButtonTheme(proxy(), widget, option, SC_TitleBarContextHelpButton, isActive, WP_HELPBUTTON, &theme);
                d->drawBackground(theme);
            }
            bool drawNormalButton = (sub & SC_TitleBarNormalButton)
                    && (((tb->titleBarFlags & Qt::WindowMinimizeButtonHint)
                         && (tb->titleBarState & Qt::WindowMinimized))
                        || ((tb->titleBarFlags & Qt::WindowMaximizeButtonHint)
                            && (tb->titleBarState & Qt::WindowMaximized)));
            if (drawNormalButton) {
                populateTitleBarButtonTheme(proxy(), widget, option, SC_TitleBarNormalButton, isActive, WP_RESTOREBUTTON, &theme);
                d->drawBackground(theme);
            }
            if (sub & SC_TitleBarShadeButton && tb->titleBarFlags & Qt::WindowShadeButtonHint
                    && !(tb->titleBarState & Qt::WindowMinimized)) {
                populateTitleBarButtonTheme(proxy(), widget, option, SC_TitleBarShadeButton, isActive, WP_MINBUTTON, &theme);
                d->drawBackground(theme);
            }
            if (sub & SC_TitleBarUnshadeButton && tb->titleBarFlags & Qt::WindowShadeButtonHint
                    && tb->titleBarState & Qt::WindowMinimized) {
                populateTitleBarButtonTheme(proxy(), widget, option, SC_TitleBarUnshadeButton, isActive, WP_RESTOREBUTTON, &theme);
                d->drawBackground(theme);
            }
            if (sub & SC_TitleBarCloseButton && tb->titleBarFlags & Qt::WindowSystemMenuHint) {
                populateTitleBarButtonTheme(proxy(), widget, option, SC_TitleBarCloseButton, isActive, WP_CLOSEBUTTON, &theme);
                d->drawBackground(theme);
            }
        }
        break;

#if QT_CONFIG(mdiarea)
    case CC_MdiControls: {
        QWindowsThemeData theme(widget, painter, QWindowsVistaStylePrivate::WindowTheme, WP_MDICLOSEBUTTON, CBS_NORMAL);
        if (Q_UNLIKELY(!theme.isValid()))
            return;

        if (option->subControls.testFlag(SC_MdiCloseButton)) {
            populateMdiButtonTheme(proxy(), widget, option, SC_MdiCloseButton, WP_MDICLOSEBUTTON, &theme);
            d->drawBackground(theme, mdiButtonCorrectionFactor(theme, widget));
        }
        if (option->subControls.testFlag(SC_MdiNormalButton)) {
            populateMdiButtonTheme(proxy(), widget, option, SC_MdiNormalButton, WP_MDIRESTOREBUTTON, &theme);
            d->drawBackground(theme, mdiButtonCorrectionFactor(theme, widget));
        }
        if (option->subControls.testFlag(QStyle::SC_MdiMinButton)) {
            populateMdiButtonTheme(proxy(), widget, option, SC_MdiMinButton, WP_MDIMINBUTTON, &theme);
            d->drawBackground(theme, mdiButtonCorrectionFactor(theme, widget));
        }
        break;
    }
#endif // QT_CONFIG(mdiarea)

#if QT_CONFIG(dial)
    case CC_Dial:
        if (const auto *dial = qstyleoption_cast<const QStyleOptionSlider *>(option))
            QStyleHelper::drawDial(dial, painter);
        break;
#endif // QT_CONFIG(dial)

    case CC_ComboBox:
        if (const QStyleOptionComboBox *cmb = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            if (cmb->editable) {
                if (sub & SC_ComboBoxEditField) {
                    partId = EP_EDITBORDER_NOSCROLL;
                    if (!(flags & State_Enabled))
                        stateId = ETS_DISABLED;
                    else if (flags & State_MouseOver)
                        stateId = ETS_HOT;
                    else if (flags & State_HasFocus)
                        stateId = ETS_FOCUSED;
                    else
                        stateId = ETS_NORMAL;

                    QWindowsThemeData theme(widget, painter,
                                    QWindowsVistaStylePrivate::EditTheme,
                                    partId, stateId, r);

                    d->drawBackground(theme);
                }
                if (sub & SC_ComboBoxArrow) {
                    QRect subRect = proxy()->subControlRect(CC_ComboBox, option, SC_ComboBoxArrow, widget);
                    QWindowsThemeData theme(widget, painter, QWindowsVistaStylePrivate::ComboboxTheme);
                    theme.rect = subRect;
                    partId = option->direction == Qt::RightToLeft ? CP_DROPDOWNBUTTONLEFT : CP_DROPDOWNBUTTONRIGHT;

                    if (!(cmb->state & State_Enabled))
                        stateId = CBXS_DISABLED;
                    else if (cmb->state & State_Sunken || cmb->state & State_On)
                        stateId = CBXS_PRESSED;
                    else if (cmb->state & State_MouseOver && option->activeSubControls & SC_ComboBoxArrow)
                        stateId = CBXS_HOT;
                    else
                        stateId = CBXS_NORMAL;

                    theme.partId = partId;
                    theme.stateId = stateId;
                    d->drawBackground(theme);
                }

            } else {
                if (sub & SC_ComboBoxFrame) {
                    QWindowsThemeData theme(widget, painter, QWindowsVistaStylePrivate::ComboboxTheme);
                    theme.rect = option->rect;
                    theme.partId = CP_READONLY;
                    if (!(cmb->state & State_Enabled))
                        theme.stateId = CBXS_DISABLED;
                    else if (cmb->state & State_Sunken || cmb->state & State_On)
                        theme.stateId = CBXS_PRESSED;
                    else if (cmb->state & State_MouseOver)
                        theme.stateId = CBXS_HOT;
                    else
                        theme.stateId = CBXS_NORMAL;
                    d->drawBackground(theme);
                }
                if (sub & SC_ComboBoxArrow) {
                    QWindowsThemeData theme(widget, painter, QWindowsVistaStylePrivate::ComboboxTheme);
                    theme.rect = proxy()->subControlRect(CC_ComboBox, option, SC_ComboBoxArrow, widget);
                    theme.partId = option->direction == Qt::RightToLeft ? CP_DROPDOWNBUTTONLEFT : CP_DROPDOWNBUTTONRIGHT;
                    if (!(cmb->state & State_Enabled))
                        theme.stateId = CBXS_DISABLED;
                    else
                        theme.stateId = CBXS_NORMAL;
                    d->drawBackground(theme);
                }
                if ((sub & SC_ComboBoxEditField) && (flags & State_HasFocus)) {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=(*cmb);
                    fropt.rect = proxy()->subControlRect(CC_ComboBox, option, SC_ComboBoxEditField, widget);
                    proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
                }
            }
        }
        break;

    case CC_ScrollBar:
        if (const QStyleOptionSlider *scrollbar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            QWindowsThemeData theme(widget, painter, QWindowsVistaStylePrivate::ScrollBarTheme);
            bool maxedOut = (scrollbar->maximum == scrollbar->minimum);
            if (maxedOut)
                flags &= ~State_Enabled;

            bool isHorz = flags & State_Horizontal;
            bool isRTL  = option->direction == Qt::RightToLeft;
            if (sub & SC_ScrollBarAddLine) {
                theme.rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarAddLine, widget);
                partId = SBP_ARROWBTN;
                if (!(flags & State_Enabled))
                    stateId = (isHorz ? (isRTL ? ABS_LEFTDISABLED : ABS_RIGHTDISABLED) : ABS_DOWNDISABLED);
                else if (scrollbar->activeSubControls & SC_ScrollBarAddLine && (scrollbar->state & State_Sunken))
                    stateId = (isHorz ? (isRTL ? ABS_LEFTPRESSED : ABS_RIGHTPRESSED) : ABS_DOWNPRESSED);
                else if (scrollbar->activeSubControls & SC_ScrollBarAddLine && (scrollbar->state & State_MouseOver))
                    stateId = (isHorz ? (isRTL ? ABS_LEFTHOT : ABS_RIGHTHOT) : ABS_DOWNHOT);
                else if (scrollbar->state & State_MouseOver)
                    stateId = (isHorz ? (isRTL ? ABS_LEFTHOVER : ABS_RIGHTHOVER) : ABS_DOWNHOVER);
                else
                    stateId = (isHorz ? (isRTL ? ABS_LEFTNORMAL : ABS_RIGHTNORMAL) : ABS_DOWNNORMAL);
                theme.partId = partId;
                theme.stateId = stateId;
                d->drawBackground(theme);
            }
            if (sub & SC_ScrollBarSubLine) {
                theme.rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSubLine, widget);
                partId = SBP_ARROWBTN;
                if (!(flags & State_Enabled))
                    stateId = (isHorz ? (isRTL ? ABS_RIGHTDISABLED : ABS_LEFTDISABLED) : ABS_UPDISABLED);
                else if (scrollbar->activeSubControls & SC_ScrollBarSubLine && (scrollbar->state & State_Sunken))
                    stateId = (isHorz ? (isRTL ? ABS_RIGHTPRESSED : ABS_LEFTPRESSED) : ABS_UPPRESSED);
                else if (scrollbar->activeSubControls & SC_ScrollBarSubLine && (scrollbar->state & State_MouseOver))
                    stateId = (isHorz ? (isRTL ? ABS_RIGHTHOT : ABS_LEFTHOT) : ABS_UPHOT);
                else if (scrollbar->state & State_MouseOver)
                    stateId = (isHorz ? (isRTL ? ABS_RIGHTHOVER : ABS_LEFTHOVER) : ABS_UPHOVER);
                else
                    stateId = (isHorz ? (isRTL ? ABS_RIGHTNORMAL : ABS_LEFTNORMAL) : ABS_UPNORMAL);
                theme.partId = partId;
                theme.stateId = stateId;
                d->drawBackground(theme);
            }
            if (maxedOut) {
                theme.rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSlider, widget);
                theme.rect = theme.rect.united(proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSubPage, widget));
                theme.rect = theme.rect.united(proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarAddPage, widget));
                partId = flags & State_Horizontal ? SBP_LOWERTRACKHORZ : SBP_LOWERTRACKVERT;
                stateId = SCRBS_DISABLED;
                theme.partId = partId;
                theme.stateId = stateId;
                d->drawBackground(theme);
            } else {
                if (sub & SC_ScrollBarSubPage) {
                    theme.rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSubPage, widget);
                    partId = flags & State_Horizontal ? SBP_UPPERTRACKHORZ : SBP_UPPERTRACKVERT;
                    if (!(flags & State_Enabled))
                        stateId = SCRBS_DISABLED;
                    else if (scrollbar->activeSubControls & SC_ScrollBarSubPage && (scrollbar->state & State_Sunken))
                        stateId = SCRBS_PRESSED;
                    else if (scrollbar->activeSubControls & SC_ScrollBarSubPage && (scrollbar->state & State_MouseOver))
                        stateId = SCRBS_HOT;
                    else
                        stateId = SCRBS_NORMAL;
                    theme.partId = partId;
                    theme.stateId = stateId;
                    d->drawBackground(theme);
                }
                if (sub & SC_ScrollBarAddPage) {
                    theme.rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarAddPage, widget);
                    partId = flags & State_Horizontal ? SBP_LOWERTRACKHORZ : SBP_LOWERTRACKVERT;
                    if (!(flags & State_Enabled))
                        stateId = SCRBS_DISABLED;
                    else if (scrollbar->activeSubControls & SC_ScrollBarAddPage && (scrollbar->state & State_Sunken))
                        stateId = SCRBS_PRESSED;
                    else if (scrollbar->activeSubControls & SC_ScrollBarAddPage && (scrollbar->state & State_MouseOver))
                        stateId = SCRBS_HOT;
                    else
                        stateId = SCRBS_NORMAL;
                    theme.partId = partId;
                    theme.stateId = stateId;
                    d->drawBackground(theme);
                }
                if (sub & SC_ScrollBarSlider) {
                    theme.rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSlider, widget);
                    if (!(flags & State_Enabled))
                        stateId = SCRBS_DISABLED;
                    else if (scrollbar->activeSubControls & SC_ScrollBarSlider && (scrollbar->state & State_Sunken))
                        stateId = SCRBS_PRESSED;
                    else if (scrollbar->activeSubControls & SC_ScrollBarSlider && (scrollbar->state & State_MouseOver))
                        stateId = SCRBS_HOT;
                    else if (option->state & State_MouseOver)
                        stateId = SCRBS_HOVER;
                    else
                        stateId = SCRBS_NORMAL;

                    // Draw handle
                    theme.partId = flags & State_Horizontal ? SBP_THUMBBTNHORZ : SBP_THUMBBTNVERT;
                    theme.stateId = stateId;
                    d->drawBackground(theme);
                }
            }
        }
        break;

#if QT_CONFIG(spinbox)
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *sb = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            QWindowsThemeData theme(widget, painter, QWindowsVistaStylePrivate::SpinTheme);
            if (sb->frame && (sub & SC_SpinBoxFrame)) {
                partId = EP_EDITBORDER_NOSCROLL;
                if (!(flags & State_Enabled))
                    stateId = ETS_DISABLED;
                else if (flags & State_MouseOver)
                    stateId = ETS_HOT;
                else if (flags & State_HasFocus)
                    stateId = ETS_SELECTED;
                else
                    stateId = ETS_NORMAL;

                QWindowsThemeData ftheme(widget, painter,
                                 QWindowsVistaStylePrivate::EditTheme,
                                 partId, stateId, r);
                // The spinbox in Windows QStyle is drawn with frameless QLineEdit inside it
                // That however breaks with QtQuickControls where this results in transparent
                // spinbox background, so if there's no "widget" passed (QtQuickControls case),
                // let ftheme.noContent be false, which fixes the spinbox rendering in QQC
                ftheme.noContent = (widget != nullptr);
                d->drawBackground(ftheme);
            }
            if (sub & SC_SpinBoxUp) {
                theme.rect = proxy()->subControlRect(CC_SpinBox, option, SC_SpinBoxUp, widget).adjusted(0, 0, 0, 1);
                partId = SPNP_UP;
                if (!(sb->stepEnabled & QAbstractSpinBox::StepUpEnabled) || !(flags & State_Enabled))
                    stateId = UPS_DISABLED;
                else if (sb->activeSubControls == SC_SpinBoxUp && (sb->state & State_Sunken))
                    stateId = UPS_PRESSED;
                else if (sb->activeSubControls == SC_SpinBoxUp && (sb->state & State_MouseOver))
                    stateId = UPS_HOT;
                else
                    stateId = UPS_NORMAL;
                theme.partId = partId;
                theme.stateId = stateId;
                d->drawBackground(theme);
            }
            if (sub & SC_SpinBoxDown) {
                theme.rect = proxy()->subControlRect(CC_SpinBox, option, SC_SpinBoxDown, widget);
                partId = SPNP_DOWN;
                if (!(sb->stepEnabled & QAbstractSpinBox::StepDownEnabled) || !(flags & State_Enabled))
                    stateId = DNS_DISABLED;
                else if (sb->activeSubControls == SC_SpinBoxDown && (sb->state & State_Sunken))
                    stateId = DNS_PRESSED;
                else if (sb->activeSubControls == SC_SpinBoxDown && (sb->state & State_MouseOver))
                    stateId = DNS_HOT;
                else
                    stateId = DNS_NORMAL;
                theme.partId = partId;
                theme.stateId = stateId;
                d->drawBackground(theme);
            }
        }
        break;
#endif // QT_CONFIG(spinbox)

    default:
        QWindowsStyle::drawComplexControl(control, option, painter, widget);
        break;
    }
}

/*!
 \internal
 */
QSize QWindowsVistaStyle::sizeFromContents(ContentsType type, const QStyleOption *option,
                                           const QSize &size, const QWidget *widget) const
{
    if (!QWindowsVistaStylePrivate::useVista())
        return QWindowsStyle::sizeFromContents(type, option, size, widget);

    QSize contentSize(size);

    switch (type) {
    case CT_LineEdit:
    case CT_ComboBox: {
        QWindowsThemeData buttontheme(widget, nullptr, QWindowsVistaStylePrivate::ButtonTheme, BP_PUSHBUTTON, PBS_NORMAL);
        if (buttontheme.isValid()) {
            const qreal factor = QWindowsStylePrivate::nativeMetricScaleFactor(widget);
            const QMarginsF borderSize = buttontheme.margins() * factor;
            if (!borderSize.isNull()) {
                const qreal margin = qreal(2) * factor;
                contentSize.rwidth() += qRound(borderSize.left() + borderSize.right() - margin);
                contentSize.rheight() += int(borderSize.bottom() + borderSize.top() - margin
                                             + qreal(1) / factor - 1);
            }
            const int textMargins = 2*(proxy()->pixelMetric(PM_FocusFrameHMargin, option) + 1);
            contentSize += QSize(qMax(pixelMetric(QStyle::PM_ScrollBarExtent, option, widget)
                                      + textMargins, 23), 0); //arrow button
        }
        break;
    }

    case CT_TabWidget:
        contentSize += QSize(6, 6);
        break;

    case CT_Menu:
        contentSize += QSize(1, 0);
        break;

#if QT_CONFIG(menubar)
    case CT_MenuBarItem:
        if (!contentSize.isEmpty())
            contentSize += QSize(windowsItemHMargin * 5 + 1, 5);
        break;
#endif

    case CT_MenuItem: {
        contentSize = QWindowsStyle::sizeFromContents(type, option, size, widget);
        QWindowsThemeData theme(widget, nullptr,
                                QWindowsVistaStylePrivate::MenuTheme,
                                MENU_POPUPCHECKBACKGROUND, MBI_HOT);
        QWindowsThemeData themeSize = theme;
        themeSize.partId = MENU_POPUPCHECK;
        themeSize.stateId = 0;
        const QSizeF size = themeSize.size() * QWindowsStylePrivate::nativeMetricScaleFactor(widget);
        const QMarginsF margins = themeSize.margins() * QWindowsStylePrivate::nativeMetricScaleFactor(widget);
        int minimumHeight = qMax(qRound(size.height() + margins.bottom() + margins.top()), contentSize.height());
        contentSize.rwidth() += qRound(size.width() + margins.left() + margins.right());
        if (const QStyleOptionMenuItem *menuitem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            if (menuitem->menuItemType != QStyleOptionMenuItem::Separator)
                contentSize.setHeight(minimumHeight);
        }
        break;
    }

    case CT_MdiControls: {
        contentSize.setHeight(int(QStyleHelper::dpiScaled(19, option)));
        int width = 54;
        if (const auto *styleOpt = qstyleoption_cast<const QStyleOptionComplex *>(option)) {
            width = 0;
            if (styleOpt->subControls & SC_MdiMinButton)
                width += 17 + 1;
            if (styleOpt->subControls & SC_MdiNormalButton)
                width += 17 + 1;
            if (styleOpt->subControls & SC_MdiCloseButton)
                width += 17 + 1;
        }
        contentSize.setWidth(int(QStyleHelper::dpiScaled(width, option)));
        break;
    }

    case CT_ItemViewItem:
        contentSize = QWindowsStyle::sizeFromContents(type, option, size, widget);
        contentSize.rheight() += 2;
        break;

    case CT_SpinBox: {
        //Spinbox adds frame twice
        contentSize = QWindowsStyle::sizeFromContents(type, option, size, widget);
        int border = proxy()->pixelMetric(PM_SpinBoxFrameWidth, option, widget);
        contentSize -= QSize(2*border, 2*border);
        break;
    }

    case CT_HeaderSection:
        // When there is a sort indicator it adds to the width but it is shown
        // above the text natively and not on the side
        if (QStyleOptionHeader *hdr = qstyleoption_cast<QStyleOptionHeader *>(const_cast<QStyleOption *>(option))) {
            QStyleOptionHeader::SortIndicator sortInd = hdr->sortIndicator;
            hdr->sortIndicator = QStyleOptionHeader::None;
            contentSize = QWindowsStyle::sizeFromContents(type, hdr, size, widget);
            hdr->sortIndicator = sortInd;
        }
        break;

    default:
        contentSize = QWindowsStyle::sizeFromContents(type, option, size, widget);
        break;
    }

    return contentSize;
}

/*!
 \internal
 */
QRect QWindowsVistaStyle::subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const
{
    if (!QWindowsVistaStylePrivate::useVista())
        return QWindowsStyle::subElementRect(element, option, widget);

    QRect rect(option->rect);

    switch (element) {
    case SE_PushButtonContents:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            MARGINS borderSize;
            const HTHEME theme = OpenThemeData(widget ? QWindowsVistaStylePrivate::winId(widget) : nullptr, L"Button");
            if (theme) {
                int stateId = PBS_NORMAL;
                if (!(option->state & State_Enabled))
                    stateId = PBS_DISABLED;
                else if (option->state & State_Sunken)
                    stateId = PBS_PRESSED;
                else if (option->state & State_MouseOver)
                    stateId = PBS_HOT;
                else if (btn->features & QStyleOptionButton::DefaultButton)
                    stateId = PBS_DEFAULTED;

                int border = proxy()->pixelMetric(PM_DefaultFrameWidth, btn, widget);
                rect = option->rect.adjusted(border, border, -border, -border);

                if (SUCCEEDED(GetThemeMargins(theme, nullptr, BP_PUSHBUTTON, stateId, TMT_CONTENTMARGINS, nullptr, &borderSize))) {
                    rect.adjust(borderSize.cxLeftWidth, borderSize.cyTopHeight,
                                -borderSize.cxRightWidth, -borderSize.cyBottomHeight);
                    rect = visualRect(option->direction, option->rect, rect);
                }
            }
        }
        break;

    case SE_DockWidgetCloseButton:
    case SE_DockWidgetFloatButton:
        rect = QWindowsStyle::subElementRect(element, option, widget);
        return rect.translated(0, 1);

    case SE_TabWidgetTabContents:
        rect = QWindowsStyle::subElementRect(element, option, widget);
#if QT_CONFIG(tabwidget)
        if (qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(option))  {
            rect = QWindowsStyle::subElementRect(element, option, widget);
            if (const QTabWidget *tabWidget = qobject_cast<const QTabWidget *>(widget)) {
                if (tabWidget->documentMode())
                    break;
                rect.adjust(0, 0, -2, -2);
            }
        }
#endif // QT_CONFIG(tabwidget)
        break;

    case SE_TabWidgetTabBar: {
        rect = QWindowsStyle::subElementRect(element, option, widget);
#if QT_CONFIG(tabwidget)
        const auto *twfOption = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(option);
        if (twfOption && twfOption->direction == Qt::RightToLeft
                && (twfOption->shape == QTabBar::RoundedNorth
                    || twfOption->shape == QTabBar::RoundedSouth))
        {
            QStyleOptionTab otherOption;
            otherOption.shape = (twfOption->shape == QTabBar::RoundedNorth
                                 ? QTabBar::RoundedEast : QTabBar::RoundedSouth);
            int overlap = proxy()->pixelMetric(PM_TabBarBaseOverlap, &otherOption, widget);
            int borderThickness = proxy()->pixelMetric(PM_DefaultFrameWidth, option, widget);
            rect.adjust(-overlap + borderThickness, 0, -overlap + borderThickness, 0);
        }
#endif // QT_CONFIG(tabwidget)
        break;
    }

    case SE_HeaderArrow: {
        rect = QWindowsStyle::subElementRect(element, option, widget);
        QRect r = rect;
        int h = option->rect.height();
        int w = option->rect.width();
        int x = option->rect.x();
        int y = option->rect.y();
        int margin = proxy()->pixelMetric(QStyle::PM_HeaderMargin, option, widget);

        QWindowsThemeData theme(widget, nullptr,
                        QWindowsVistaStylePrivate::HeaderTheme,
                        HP_HEADERSORTARROW, HSAS_SORTEDDOWN, option->rect);

        int arrowWidth = 13;
        int arrowHeight = 5;
        if (theme.isValid()) {
            const QSizeF size = theme.size() * QWindowsStylePrivate::nativeMetricScaleFactor(widget);
            if (!size.isEmpty()) {
                arrowWidth = qRound(size.width());
                arrowHeight = qRound(size.height());
            }
        }
        if (option->state & State_Horizontal) {
            r.setRect(x + w/2 - arrowWidth/2, y , arrowWidth, arrowHeight);
        } else {
            int vert_size = w / 2;
            r.setRect(x + 5, y + h - margin * 2 - vert_size,
                      w - margin * 2 - 5, vert_size);
        }
        rect = visualRect(option->direction, option->rect, r);
        break;
    }

    case SE_HeaderLabel: {
        rect = QWindowsStyle::subElementRect(element, option, widget);
        int margin = proxy()->pixelMetric(QStyle::PM_HeaderMargin, option, widget);
        QRect r = option->rect;
        r.setRect(option->rect.x() + margin, option->rect.y() + margin,
                  option->rect.width() - margin * 2, option->rect.height() - margin * 2);
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            // Subtract width needed for arrow, if there is one
            if (header->sortIndicator != QStyleOptionHeader::None) {
                if (!(option->state & State_Horizontal)) //horizontal arrows are positioned on top
                    r.setHeight(r.height() - (option->rect.width() / 2) - (margin * 2));
            }
        }
        rect = visualRect(option->direction, option->rect, r);
        break;
    }

    case SE_ProgressBarContents:
        rect = QCommonStyle::subElementRect(SE_ProgressBarGroove, option, widget);
        break;

    case SE_ItemViewItemFocusRect:
        rect = QWindowsStyle::subElementRect(element, option, widget);
        if (const QStyleOptionViewItem *vopt = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
            QRect textRect = subElementRect(QStyle::SE_ItemViewItemText, option, widget);
            QRect displayRect = subElementRect(QStyle::SE_ItemViewItemDecoration, option, widget);
            if (!vopt->icon.isNull())
                rect = textRect.united(displayRect);
            else
                rect = textRect;
            rect = rect.adjusted(1, 0, -1, 0);
        }
        break;

    default:
        rect = QWindowsStyle::subElementRect(element, option, widget);
        break;
    }

    return rect;
}

/*!
 \internal
 */
QRect QWindowsVistaStyle::subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                                         SubControl subControl, const QWidget *widget) const
{
    if (!QWindowsVistaStylePrivate::useVista())
        return QWindowsStyle::subControlRect(control, option, subControl, widget);

    QRect rect;

    switch (control) {
#if QT_CONFIG(combobox)
    case CC_ComboBox:
        if (const auto *cb = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            const int x = cb->rect.x(), y = cb->rect.y(), wi = cb->rect.width(), he = cb->rect.height();
            const int margin = cb->frame ? 3 : 0;
            const int bmarg = cb->frame ? 2 : 0;
            const int arrowWidth = qRound(QStyleHelper::dpiScaled(16, option));
            const int arrowButtonWidth = bmarg + arrowWidth;
            const int xpos = x + wi - arrowButtonWidth;

            switch (subControl) {
            case SC_ComboBoxFrame:
            case SC_ComboBoxListBoxPopup:
                rect = cb->rect;
                break;

            case SC_ComboBoxArrow:  {
                rect.setRect(xpos, y , arrowButtonWidth, he);
            }
                break;

            case SC_ComboBoxEditField:  {
                rect.setRect(x + margin, y + margin, wi - 2 * margin - arrowWidth, he - 2 * margin);
            }
                break;

            default:
                break;
            }
        }
        break;
#endif // QT_CONFIG(combobox)

    case CC_TitleBar:
        if (const QStyleOptionTitleBar *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(option)) {
            if (!buttonVisible(subControl, tb))
                return rect;
            const qreal factor = QWindowsStylePrivate::nativeMetricScaleFactor(widget);
            const bool isToolTitle = false;
            const int height = tb->rect.height();
            const int width = tb->rect.width();

            const int buttonMargin = int(QStyleHelper::dpiScaled(4, option));
            int buttonHeight = qRound(qreal(GetSystemMetrics(SM_CYSIZE)) * factor)
                    - buttonMargin;
            const int buttonWidth =
                    qRound(qreal(GetSystemMetrics(SM_CXSIZE)) * factor - QStyleHelper::dpiScaled(4, option));

            const int frameWidth = proxy()->pixelMetric(PM_MdiSubWindowFrameWidth, option, widget);
            const bool sysmenuHint  = (tb->titleBarFlags & Qt::WindowSystemMenuHint) != 0;
            const bool minimizeHint = (tb->titleBarFlags & Qt::WindowMinimizeButtonHint) != 0;
            const bool maximizeHint = (tb->titleBarFlags & Qt::WindowMaximizeButtonHint) != 0;
            const bool contextHint = (tb->titleBarFlags & Qt::WindowContextHelpButtonHint) != 0;
            const bool shadeHint = (tb->titleBarFlags & Qt::WindowShadeButtonHint) != 0;

            bool isMinimized = tb->titleBarState & Qt::WindowMinimized;
            bool isMaximized = tb->titleBarState & Qt::WindowMaximized;
            int offset = 0;
            const int delta = buttonWidth + 2;
            int controlTop = option->rect.bottom() - buttonHeight - 2;

            switch (subControl) {
            case SC_TitleBarLabel:  {
                rect = QRect(frameWidth, 0, width - (buttonWidth + frameWidth + 10), height);
                if (isToolTitle) {
                    if (sysmenuHint) {
                        rect.adjust(0, 0, int(-buttonWidth - 3 * factor), 0);
                    }
                    if (minimizeHint || maximizeHint)
                        rect.adjust(0, 0, int(-buttonWidth - 2 * factor), 0);
                } else {
                    if (sysmenuHint) {
                        const int leftOffset = int(height - 8 * factor);
                        rect.adjust(leftOffset, 0, 0, int(4 * factor));
                    }
                    if (minimizeHint)
                        rect.adjust(0, 0, int(-buttonWidth - 2 * factor), 0);
                    if (maximizeHint)
                        rect.adjust(0, 0, int(-buttonWidth - 2 * factor), 0);
                    if (contextHint)
                        rect.adjust(0, 0, int(-buttonWidth - 2 * factor), 0);
                    if (shadeHint)
                        rect.adjust(0, 0, int(-buttonWidth - 2 * factor), 0);
                }
                rect.translate(0, int(2 * factor));
            }
                break;

            case SC_TitleBarContextHelpButton:
                if (tb->titleBarFlags & Qt::WindowContextHelpButtonHint)
                    offset += delta;
                Q_FALLTHROUGH();
            case SC_TitleBarMinButton:
                if (!isMinimized && (tb->titleBarFlags & Qt::WindowMinimizeButtonHint))
                    offset += delta;
                else if (subControl == SC_TitleBarMinButton)
                    break;
                Q_FALLTHROUGH();
            case SC_TitleBarNormalButton:
                if (isMinimized && (tb->titleBarFlags & Qt::WindowMinimizeButtonHint))
                    offset += delta;
                else if (isMaximized && (tb->titleBarFlags & Qt::WindowMaximizeButtonHint))
                    offset += delta;
                else if (subControl == SC_TitleBarNormalButton)
                    break;
                Q_FALLTHROUGH();
            case SC_TitleBarMaxButton:
                if (!isMaximized && (tb->titleBarFlags & Qt::WindowMaximizeButtonHint))
                    offset += delta;
                else if (subControl == SC_TitleBarMaxButton)
                    break;
                Q_FALLTHROUGH();
            case SC_TitleBarShadeButton:
                if (!isMinimized && (tb->titleBarFlags & Qt::WindowShadeButtonHint))
                    offset += delta;
                else if (subControl == SC_TitleBarShadeButton)
                    break;
                Q_FALLTHROUGH();
            case SC_TitleBarUnshadeButton:
                if (isMinimized && (tb->titleBarFlags & Qt::WindowShadeButtonHint))
                    offset += delta;
                else if (subControl == SC_TitleBarUnshadeButton)
                    break;
                Q_FALLTHROUGH();
            case SC_TitleBarCloseButton:
                if (tb->titleBarFlags & Qt::WindowSystemMenuHint)
                    offset += delta;
                else if (subControl == SC_TitleBarCloseButton)
                    break;

                rect.setRect(width - offset - controlTop + 1, controlTop,
                             buttonWidth, buttonHeight);
                break;

            case SC_TitleBarSysMenu:    {
                const int controlTop = int(6 * factor);
                const int controlHeight = int(height - controlTop - 3 * factor);
                int iconExtent = proxy()->pixelMetric(PM_SmallIconSize, option);
                QSize iconSize = tb->icon.actualSize(QSize(iconExtent, iconExtent));
                if (tb->icon.isNull())
                    iconSize = QSize(controlHeight, controlHeight);
                int hPad = (controlHeight - iconSize.height())/2;
                int vPad = (controlHeight - iconSize.width())/2;
                rect = QRect(frameWidth + hPad, controlTop + vPad, iconSize.width(), iconSize.height());
                rect.translate(0, int(3 * factor));
            }
                break;

            default:
                break;
            }
        }
        break;

#if QT_CONFIG(mdiarea)
    case CC_MdiControls: {
        int numSubControls = 0;
        if (option->subControls & SC_MdiCloseButton)
            ++numSubControls;
        if (option->subControls & SC_MdiMinButton)
            ++numSubControls;
        if (option->subControls & SC_MdiNormalButton)
            ++numSubControls;
        if (numSubControls == 0)
            break;

        int buttonWidth = option->rect.width() / numSubControls;
        int offset = 0;

        switch (subControl) {
        case SC_MdiCloseButton:
            // Only one sub control, no offset needed.
            if (numSubControls == 1)
                break;
            offset += buttonWidth;
            Q_FALLTHROUGH();
        case SC_MdiNormalButton:
            // No offset needed if
            // 1) There's only one sub control
            // 2) We have a close button and a normal button (offset already added in SC_MdiClose)
            if (numSubControls == 1 || (numSubControls == 2 && !(option->subControls & SC_MdiMinButton)))
                break;
            if (option->subControls & SC_MdiNormalButton)
                offset += buttonWidth;
            break;
        default:
            break;
        }

        rect = QRect(offset, 0, buttonWidth, option->rect.height());
        break;
    }
#endif // QT_CONFIG(mdiarea)

    default:
        return QWindowsStyle::subControlRect(control, option, subControl, widget);
    }

    return visualRect(option->direction, option->rect, rect);
}

/*!
 \internal
 */
QStyle::SubControl QWindowsVistaStyle::hitTestComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                                             const QPoint &pos, const QWidget *widget) const
{
    return QWindowsStyle::hitTestComplexControl(control, option, pos, widget);
}

/*!
 \internal
 */
int QWindowsVistaStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    if (!QWindowsVistaStylePrivate::useVista())
        return QWindowsStyle::pixelMetric(metric, option, widget);

    int ret = QWindowsVistaStylePrivate::fixedPixelMetric(metric);
    if (ret != QWindowsStylePrivate::InvalidMetric)
        return int(QStyleHelper::dpiScaled(ret, option));

    int res = QWindowsVistaStylePrivate::pixelMetricFromSystemDp(metric, option, widget);
    if (res != QWindowsStylePrivate::InvalidMetric)
        return qRound(qreal(res) * QWindowsStylePrivate::nativeMetricScaleFactor(widget));

    res = 0;

    switch (metric) {
    case PM_MenuBarPanelWidth:
    case PM_ButtonDefaultIndicator:
        res = 0;
        break;

    case PM_DefaultFrameWidth:
        res = qobject_cast<const QListView*>(widget) ? 2 : 1;
        break;
    case PM_MenuPanelWidth:
    case PM_SpinBoxFrameWidth:
        res = 1;
        break;

    case PM_TabBarTabOverlap:
    case PM_MenuHMargin:
    case PM_MenuVMargin:
        res = 2;
        break;

#if QT_CONFIG(tabbar)
    case PM_TabBarBaseOverlap:
        if (const auto *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {
            switch (tab->shape) {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
                res = 1;
                break;
            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
                res = 2;
                break;
            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
                res = 3;
                break;
            }
        }
        break;
#endif  // QT_CONFIG(tabbar)

    case PM_SplitterWidth:
        res = QStyleHelper::dpiScaled(5., option);
        break;

    case PM_MdiSubWindowMinimizedWidth:
        res = 160;
        break;

#if QT_CONFIG(toolbar)
    case PM_ToolBarHandleExtent:
        res = int(QStyleHelper::dpiScaled(8., option));
        break;

#endif // QT_CONFIG(toolbar)
    case PM_DockWidgetSeparatorExtent:
    case PM_DockWidgetTitleMargin:
        res = int(QStyleHelper::dpiScaled(4., option));
        break;

#if QT_CONFIG(toolbutton)
    case PM_ButtonShiftHorizontal:
    case PM_ButtonShiftVertical:
        res = qstyleoption_cast<const QStyleOptionToolButton *>(option) ? 1 : 0;
        break;
#endif  // QT_CONFIG(toolbutton)

    default:
        res = QWindowsStyle::pixelMetric(metric, option, widget);
    }

    return res;
}

/*!
 \internal
 */
void QWindowsVistaStyle::polish(QWidget *widget)
{
    QWindowsStyle::polish(widget);
    if (false
#if QT_CONFIG(abstractbutton)
        || qobject_cast<QAbstractButton*>(widget)
#endif // QT_CONFIG(abstractbutton)
#if QT_CONFIG(toolbutton)
        || qobject_cast<QToolButton*>(widget)
#endif // QT_CONFIG(toolbutton)
#if QT_CONFIG(tabbar)
        || qobject_cast<QTabBar*>(widget)
#endif // QT_CONFIG(tabbar)
#if QT_CONFIG(combobox)
        || qobject_cast<QComboBox*>(widget)
#endif // QT_CONFIG(combobox)
        || qobject_cast<QScrollBar*>(widget)
        || qobject_cast<QSlider*>(widget)
        || qobject_cast<QHeaderView*>(widget)
#if QT_CONFIG(spinbox)
        || qobject_cast<QAbstractSpinBox*>(widget)
        || qobject_cast<QSpinBox*>(widget)
#endif // QT_CONFIG(spinbox)
#if QT_CONFIG(lineedit)
        || qobject_cast<QLineEdit*>(widget)
#endif // QT_CONFIG(lineedit)
        || qobject_cast<QGroupBox*>(widget)
            ) {
        widget->setAttribute(Qt::WA_Hover);
    } else if (QTreeView *tree = qobject_cast<QTreeView *> (widget)) {
        tree->viewport()->setAttribute(Qt::WA_Hover);
    } else if (QListView *list = qobject_cast<QListView *> (widget)) {
        list->viewport()->setAttribute(Qt::WA_Hover);
    }

    if (!QWindowsVistaStylePrivate::useVista())
        return;

#if QT_CONFIG(rubberband)
    if (qobject_cast<QRubberBand*>(widget))
        widget->setWindowOpacity(0.6);
    else
#endif
#if QT_CONFIG(commandlinkbutton)
        if (qobject_cast<QCommandLinkButton*>(widget)) {
                widget->setProperty("_qt_usingVistaStyle", true);
                QFont buttonFont = widget->font();
                buttonFont.setFamilies(QStringList{QLatin1String("Segoe UI")});
                widget->setFont(buttonFont);
                QPalette pal = widget->palette();
                pal.setColor(QPalette::Active, QPalette::ButtonText, QColor(21, 28, 85));
                pal.setColor(QPalette::Active, QPalette::BrightText, QColor(7, 64, 229));
                widget->setPalette(pal);
        } else
#endif // QT_CONFIG(commandlinkbutton)
        if (widget->inherits("QTipLabel")) {
            //note that since tooltips are not reused
            //we do not have to care about unpolishing
            widget->setContentsMargins(3, 0, 4, 0);
            COLORREF bgRef;
            HTHEME theme = OpenThemeData(widget ? QWindowsVistaStylePrivate::winId(widget) : nullptr, L"TOOLTIP");
            if (theme && SUCCEEDED(GetThemeColor(theme, TTP_STANDARD, TTSS_NORMAL, TMT_TEXTCOLOR, &bgRef))) {
                QColor textColor = QColor::fromRgb(bgRef);
                QPalette pal;
                pal.setColor(QPalette::All, QPalette::ToolTipText, textColor);
                pal.setResolveMask(0);
                widget->setPalette(pal);
            }
        } else if (qobject_cast<QMessageBox *> (widget)) {
            widget->setAttribute(Qt::WA_StyledBackground);
#if QT_CONFIG(dialogbuttonbox)
            QDialogButtonBox *buttonBox = widget->findChild<QDialogButtonBox *>(QLatin1String("qt_msgbox_buttonbox"));
            if (buttonBox)
                buttonBox->setContentsMargins(0, 9, 0, 0);
#endif
        }
}

/*!
 \internal
 */
void QWindowsVistaStyle::unpolish(QWidget *widget)
{
    Q_D(QWindowsVistaStyle);

#if QT_CONFIG(rubberband)
    if (qobject_cast<QRubberBand*>(widget))
        widget->setWindowOpacity(1.0);
#endif

    // Unpolish of widgets is the first thing that
    // happens when a theme changes, or the theme
    // engine is turned off. So we detect it here.
    bool oldState = QWindowsVistaStylePrivate::useVista();
    bool newState = QWindowsVistaStylePrivate::useVista(true);
    if ((oldState != newState) && newState) {
        d->cleanup(true);
        d->init(true);
    } else {
        // Cleanup handle map, if just changing style,
        // or turning it on. In both cases the values
        // already in the map might be old (other style).
        d->cleanupHandleMap();
    }
    if (false
        #if QT_CONFIG(abstractbutton)
            || qobject_cast<QAbstractButton*>(widget)
        #endif
        #if QT_CONFIG(toolbutton)
            || qobject_cast<QToolButton*>(widget)
        #endif // QT_CONFIG(toolbutton)
        #if QT_CONFIG(tabbar)
            || qobject_cast<QTabBar*>(widget)
        #endif // QT_CONFIG(tabbar)
        #if QT_CONFIG(combobox)
            || qobject_cast<QComboBox*>(widget)
        #endif // QT_CONFIG(combobox)
            || qobject_cast<QScrollBar*>(widget)
            || qobject_cast<QSlider*>(widget)
            || qobject_cast<QHeaderView*>(widget)
        #if QT_CONFIG(spinbox)
            || qobject_cast<QAbstractSpinBox*>(widget)
            || qobject_cast<QSpinBox*>(widget)
        #endif // QT_CONFIG(spinbox)
            ) {
        widget->setAttribute(Qt::WA_Hover, false);
    }

    QWindowsStyle::unpolish(widget);

    d->stopAnimation(widget);

#if QT_CONFIG(lineedit)
    if (qobject_cast<QLineEdit*>(widget))
        widget->setAttribute(Qt::WA_Hover, false);
    else {
#endif // QT_CONFIG(lineedit)
        if (qobject_cast<QGroupBox*>(widget))
            widget->setAttribute(Qt::WA_Hover, false);
        else if (qobject_cast<QMessageBox *> (widget)) {
            widget->setAttribute(Qt::WA_StyledBackground, false);
#if QT_CONFIG(dialogbuttonbox)
            QDialogButtonBox *buttonBox = widget->findChild<QDialogButtonBox *>(QLatin1String("qt_msgbox_buttonbox"));
            if (buttonBox)
                buttonBox->setContentsMargins(0, 0, 0, 0);
#endif
        }
        else if (QTreeView *tree = qobject_cast<QTreeView *> (widget)) {
            tree->viewport()->setAttribute(Qt::WA_Hover, false);
        }
#if QT_CONFIG(commandlinkbutton)
        else if (qobject_cast<QCommandLinkButton*>(widget)) {
            QFont font = QApplication::font("QCommandLinkButton");
            QFont widgetFont = widget->font();
            widgetFont.setFamilies(font.families()); //Only family set by polish
            widget->setFont(widgetFont);
        }
#endif // QT_CONFIG(commandlinkbutton)
    }
}

/*!
 \internal
 */
void QWindowsVistaStyle::polish(QPalette &pal)
{
    Q_D(QWindowsVistaStyle);

    if (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
        // System runs in dark mode, but the Vista style cannot use a dark palette.
        // Overwrite with the light system palette.
        using QWindowsApplication = QNativeInterface::Private::QWindowsApplication;
        if (auto nativeWindowsApp = dynamic_cast<QWindowsApplication *>(QGuiApplicationPrivate::platformIntegration()))
            nativeWindowsApp->populateLightSystemPalette(pal);
    }

    QPixmapCache::clear();
    d->alphaCache.clear();
    d->hasInitColors = false;

    if (!d->hasInitColors) {
        // Get text color for group box labels
        QWindowsThemeData theme(nullptr, nullptr, QWindowsVistaStylePrivate::ButtonTheme, 0, 0);
        COLORREF cref;
        GetThemeColor(theme.handle(), BP_GROUPBOX, GBS_NORMAL, TMT_TEXTCOLOR, &cref);
        d->groupBoxTextColor = qRgb(GetRValue(cref), GetGValue(cref), GetBValue(cref));
        GetThemeColor(theme.handle(), BP_GROUPBOX, GBS_DISABLED, TMT_TEXTCOLOR, &cref);
        d->groupBoxTextColorDisabled = qRgb(GetRValue(cref), GetGValue(cref), GetBValue(cref));
        //Work around Windows API returning the same color for enabled and disabled group boxes
        if (d->groupBoxTextColor == d->groupBoxTextColorDisabled)
             d->groupBoxTextColorDisabled = pal.color(QPalette::Disabled, QPalette::ButtonText).rgb();
        // Where does this color come from?
        //GetThemeColor(theme.handle(), TKP_TICS, TSS_NORMAL, TMT_COLOR, &cref);
        d->sliderTickColor = qRgb(165, 162, 148);
        d->hasInitColors = true;
    }

    QWindowsStyle::polish(pal);
    pal.setBrush(QPalette::AlternateBase, pal.base().color().darker(104));
}

/*!
 \internal
 */
void QWindowsVistaStyle::polish(QApplication *app)
{
    // Override windows theme palettes to light
    if (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
        static constexpr const char* themedWidgets[] = {
            "QToolButton",
            "QAbstractButton",
            "QCheckBox",
            "QRadioButton",
            "QHeaderView",
            "QAbstractItemView",
            "QMessageBoxLabel",
            "QTabBar",
            "QLabel",
            "QGroupBox",
            "QMenu",
            "QMenuBar",
            "QTextEdit",
            "QTextControl",
            "QLineEdit"
        };
        for (const auto& themedWidget : std::as_const(themedWidgets)) {
            auto defaultResolveMask = QApplication::palette().resolveMask();
            auto widgetResolveMask = QApplication::palette(themedWidget).resolveMask();
            if (widgetResolveMask != defaultResolveMask)
                QApplication::setPalette(QApplication::palette(), themedWidget);
        }
    }

    QWindowsStyle::polish(app);
}

/*!
 \internal
 */
QPixmap QWindowsVistaStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption *option,
                                           const QWidget *widget) const
{
    if (!QWindowsVistaStylePrivate::useVista()) {
        return QWindowsStyle::standardPixmap(standardPixmap, option, widget);
    }

    switch (standardPixmap) {
    case SP_TitleBarMaxButton:
    case SP_TitleBarCloseButton:
        if (qstyleoption_cast<const QStyleOptionDockWidget *>(option)) {
            if (widget && widget->isWindow()) {
                QWindowsThemeData theme(widget, nullptr, QWindowsVistaStylePrivate::WindowTheme, WP_SMALLCLOSEBUTTON, CBS_NORMAL);
                if (theme.isValid()) {
                    const QSize size = (theme.size() * QWindowsStylePrivate::nativeMetricScaleFactor(widget)).toSize();
                    return QIcon(QWindowsStyle::standardPixmap(standardPixmap, option, widget)).pixmap(size);
                }
            }
        }
        break;

    default:
        break;
    }

    return QWindowsStyle::standardPixmap(standardPixmap, option, widget);
}

/*!
\reimp
*/
QIcon QWindowsVistaStyle::standardIcon(StandardPixmap standardIcon,
                                       const QStyleOption *option,
                                       const QWidget *widget) const
{
    if (!QWindowsVistaStylePrivate::useVista()) {
        return QWindowsStyle::standardIcon(standardIcon, option, widget);
    }

    auto *d = const_cast<QWindowsVistaStylePrivate*>(d_func());

    switch (standardIcon) {
    case SP_TitleBarMaxButton:
        if (qstyleoption_cast<const QStyleOptionDockWidget *>(option)) {
            if (d->m_titleBarMaxIcon.isNull()) {
                QWindowsThemeData themeSize(nullptr, nullptr, QWindowsVistaStylePrivate::WindowTheme,
                                    WP_SMALLCLOSEBUTTON, CBS_NORMAL);
                QWindowsThemeData theme(nullptr, nullptr, QWindowsVistaStylePrivate::WindowTheme,
                                WP_MAXBUTTON, MAXBS_NORMAL);
                if (theme.isValid()) {
                    const QSize size = (themeSize.size() * QWindowsStylePrivate::nativeMetricScaleFactor(widget)).toSize();
                    QPixmap pm(size);
                    pm.fill(Qt::transparent);
                    QPainter p(&pm);
                    theme.painter = &p;
                    theme.rect = QRect(QPoint(0, 0), size);
                    d->drawBackground(theme);
                    d->m_titleBarMaxIcon.addPixmap(pm, QIcon::Normal, QIcon::Off);    // Normal
                    pm.fill(Qt::transparent);
                    theme.stateId = MAXBS_PUSHED;
                    d->drawBackground(theme);
                    d->m_titleBarMaxIcon.addPixmap(pm, QIcon::Normal, QIcon::On);     // Pressed
                    pm.fill(Qt::transparent);
                    theme.stateId = MAXBS_HOT;
                    d->drawBackground(theme);
                    d->m_titleBarMaxIcon.addPixmap(pm, QIcon::Active, QIcon::Off);    // Hover
                    pm.fill(Qt::transparent);
                    theme.stateId = MAXBS_INACTIVE;
                    d->drawBackground(theme);
                    d->m_titleBarMaxIcon.addPixmap(pm, QIcon::Disabled, QIcon::Off);  // Disabled
                }
            }
            if (widget && widget->isWindow())
                return d->m_titleBarMaxIcon;
        }
        break;

    case SP_TitleBarCloseButton:
        if (qstyleoption_cast<const QStyleOptionDockWidget *>(option)) {
            if (d->m_titleBarCloseIcon.isNull()) {
                QWindowsThemeData theme(nullptr, nullptr, QWindowsVistaStylePrivate::WindowTheme,
                                WP_SMALLCLOSEBUTTON, CBS_NORMAL);
                if (theme.isValid()) {
                    const QSize size = (theme.size() * QWindowsStylePrivate::nativeMetricScaleFactor(widget)).toSize();
                    QPixmap pm(size);
                    pm.fill(Qt::transparent);
                    QPainter p(&pm);
                    theme.painter = &p;
                    theme.partId = WP_CLOSEBUTTON; // ####
                    theme.rect = QRect(QPoint(0, 0), size);
                    d->drawBackground(theme);
                    d->m_titleBarCloseIcon.addPixmap(pm, QIcon::Normal, QIcon::Off);    // Normal
                    pm.fill(Qt::transparent);
                    theme.stateId = CBS_PUSHED;
                    d->drawBackground(theme);
                    d->m_titleBarCloseIcon.addPixmap(pm, QIcon::Normal, QIcon::On);     // Pressed
                    pm.fill(Qt::transparent);
                    theme.stateId = CBS_HOT;
                    d->drawBackground(theme);
                    d->m_titleBarCloseIcon.addPixmap(pm, QIcon::Active, QIcon::Off);    // Hover
                    pm.fill(Qt::transparent);
                    theme.stateId = CBS_INACTIVE;
                    d->drawBackground(theme);
                    d->m_titleBarCloseIcon.addPixmap(pm, QIcon::Disabled, QIcon::Off);  // Disabled
                }
            }
            if (widget && widget->isWindow())
                return d->m_titleBarCloseIcon;
        }
        break;

    case SP_TitleBarNormalButton:
        if (qstyleoption_cast<const QStyleOptionDockWidget *>(option)) {
            if (d->m_titleBarNormalIcon.isNull()) {
                QWindowsThemeData themeSize(nullptr, nullptr, QWindowsVistaStylePrivate::WindowTheme,
                                    WP_SMALLCLOSEBUTTON, CBS_NORMAL);
                QWindowsThemeData theme(nullptr, nullptr, QWindowsVistaStylePrivate::WindowTheme,
                                WP_RESTOREBUTTON, RBS_NORMAL);
                if (theme.isValid()) {
                    const QSize size = (themeSize.size() * QWindowsStylePrivate::nativeMetricScaleFactor(widget)).toSize();
                    QPixmap pm(size);
                    pm.fill(Qt::transparent);
                    QPainter p(&pm);
                    theme.painter = &p;
                    theme.rect = QRect(QPoint(0, 0), size);
                    d->drawBackground(theme);
                    d->m_titleBarNormalIcon.addPixmap(pm, QIcon::Normal, QIcon::Off);    // Normal
                    pm.fill(Qt::transparent);
                    theme.stateId = RBS_PUSHED;
                    d->drawBackground(theme);
                    d->m_titleBarNormalIcon.addPixmap(pm, QIcon::Normal, QIcon::On);     // Pressed
                    pm.fill(Qt::transparent);
                    theme.stateId = RBS_HOT;
                    d->drawBackground(theme);
                    d->m_titleBarNormalIcon.addPixmap(pm, QIcon::Active, QIcon::Off);    // Hover
                    pm.fill(Qt::transparent);
                    theme.stateId = RBS_INACTIVE;
                    d->drawBackground(theme);
                    d->m_titleBarNormalIcon.addPixmap(pm, QIcon::Disabled, QIcon::Off);  // Disabled
                }
            }
            if (widget && widget->isWindow())
                return d->m_titleBarNormalIcon;
        }
        break;

    case SP_CommandLink: {
        QWindowsThemeData theme(nullptr, nullptr, QWindowsVistaStylePrivate::ButtonTheme,
                        BP_COMMANDLINKGLYPH, CMDLGS_NORMAL);
        if (theme.isValid()) {
            const QSize size = theme.size().toSize();
            QIcon linkGlyph;
            QPixmap pm(size);
            pm.fill(Qt::transparent);
            QPainter p(&pm);
            theme.painter = &p;
            theme.rect = QRect(QPoint(0, 0), size);
            d->drawBackground(theme);
            linkGlyph.addPixmap(pm, QIcon::Normal, QIcon::Off);    // Normal
            pm.fill(Qt::transparent);

            theme.stateId = CMDLGS_PRESSED;
            d->drawBackground(theme);
            linkGlyph.addPixmap(pm, QIcon::Normal, QIcon::On);     // Pressed
            pm.fill(Qt::transparent);

            theme.stateId = CMDLGS_HOT;
            d->drawBackground(theme);
            linkGlyph.addPixmap(pm, QIcon::Active, QIcon::Off);    // Hover
            pm.fill(Qt::transparent);

            theme.stateId = CMDLGS_DISABLED;
            d->drawBackground(theme);
            linkGlyph.addPixmap(pm, QIcon::Disabled, QIcon::Off);  // Disabled
            return linkGlyph;
        }
        break;
    }

    default:
        break;
    }

    return QWindowsStyle::standardIcon(standardIcon, option, widget);
}

QT_END_NAMESPACE
