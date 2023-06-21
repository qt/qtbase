// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtWidgets/private/qtwidgetsglobal_p.h>

#if QT_CONFIG(style_windowsvista)

#include "qwizard_win_p.h"
#include <private/qapplication_p.h>
#include <private/qwindowsfontdatabasebase_p.h>
#include <qpa/qplatformwindow.h>
#include <qpa/qplatformwindow_p.h>
#include "qwizard.h"
#include "qpaintengine.h"
#include "qapplication.h"
#include <QtCore/QOperatingSystemVersion>
#include <QtCore/QVariant>
#include <QtCore/QDebug>
#include <QtGui/QMouseEvent>
#include <QtGui/QWindow>
#include <QtGui/private/qhighdpiscaling_p.h>

#include <uxtheme.h>
#include <vssym32.h>
#include <dwmapi.h>

// ### move to qmargins.h
Q_DECLARE_METATYPE(QMargins)

#ifndef WM_DWMCOMPOSITIONCHANGED
#  define WM_DWMCOMPOSITIONCHANGED 0x031E
#endif

QT_BEGIN_NAMESPACE

qreal QVistaHelper::m_devicePixelRatio = 1.0;

/******************************************************************************
** QVistaBackButton
*/

QVistaBackButton::QVistaBackButton(QWidget *widget)
    : QAbstractButton(widget)
{
    setFocusPolicy(Qt::NoFocus);
    // Native dialogs use ALT-Left even in RTL mode, so do the same, even if it might be counter-intuitive.
#if QT_CONFIG(shortcut)
    setShortcut(QKeySequence(Qt::ALT | Qt::Key_Left));
#endif
}

QSize QVistaBackButton::sizeHint() const
{
    ensurePolished();
    int size = int(QStyleHelper::dpiScaled(32, this));
    int width = size, height = size;
    return QSize(width, height);
}

void QVistaBackButton::enterEvent(QEnterEvent *event)
{
    if (isEnabled())
        update();
    QAbstractButton::enterEvent(event);
}

void QVistaBackButton::leaveEvent(QEvent *event)
{
    if (isEnabled())
        update();
    QAbstractButton::leaveEvent(event);
}

void QVistaBackButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QRect r = rect();
    const HANDLE theme = OpenThemeData(0, L"Navigation");
    //RECT rect;
    QPoint origin;
    const HDC hdc = QVistaHelper::backingStoreDC(parentWidget(), &origin);
    RECT clipRect;
    int xoffset = origin.x() + QWidget::mapToParent(r.topLeft()).x() - 1;
    int yoffset = origin.y() + QWidget::mapToParent(r.topLeft()).y() - 1;
    const qreal dpr = devicePixelRatio();
    const QRect rDp = QRect(r.topLeft() * dpr, r.size() * dpr);
    const int xoffsetDp = xoffset * dpr;
    const int yoffsetDp = yoffset * dpr;

    clipRect.top = rDp.top() + yoffsetDp;
    clipRect.bottom = rDp.bottom() + yoffsetDp;
    clipRect.left = rDp.left() + xoffsetDp;
    clipRect.right = rDp.right()  + xoffsetDp;

    int state = NAV_BB_NORMAL;
    if (!isEnabled())
        state = NAV_BB_DISABLED;
    else if (isDown())
        state = NAV_BB_PRESSED;
    else if (underMouse())
        state = NAV_BB_HOT;

    DrawThemeBackground(theme, hdc,
                        layoutDirection() == Qt::LeftToRight ? NAV_BACKBUTTON : NAV_FORWARDBUTTON,
                        state, &clipRect, &clipRect);
}

/******************************************************************************
** QVistaHelper
*/

QVistaHelper::QVistaHelper(QWizard *wizard)
    : QObject(wizard)
    , pressed(false)
    , wizard(wizard)
    , backButton_(0)
{
    QVistaHelper::m_devicePixelRatio = wizard->devicePixelRatio();

    backButton_ = new QVistaBackButton(wizard);
    backButton_->hide();

    iconSpacing = QStyleHelper::dpiScaled(7, wizard);
}

QVistaHelper::~QVistaHelper() = default;

void QVistaHelper::updateCustomMargins(bool vistaMargins)
{
    using namespace QNativeInterface::Private;

    if (QWindow *window = wizard->windowHandle()) {
        // Reduce top frame to zero since we paint it ourselves. Use
        // device pixel to avoid rounding errors.
        const QMargins customMarginsDp = vistaMargins
            ? QMargins(0, -titleBarSizeDp(), 0, 0)
            : QMargins();
        const QVariant customMarginsV = QVariant::fromValue(customMarginsDp);
        // The dynamic property takes effect when creating the platform window.
        window->setProperty("_q_windowsCustomMargins", customMarginsV);
        // If a platform window exists, change via native interface.
        if (auto platformWindow = dynamic_cast<QWindowsWindow *>(window->handle()))
            platformWindow->setCustomMargins(customMarginsDp);
    }
}

void QVistaHelper::disconnectBackButton()
{
    if (backButton_) // Leave QStyleSheetStyle's connections on destroyed() intact.
        backButton_->disconnect(SIGNAL(clicked()));
}

QColor QVistaHelper::basicWindowFrameColor()
{
    DWORD rgb;
    const HANDLE hTheme = OpenThemeData(GetDesktopWindow(), L"WINDOW");
    GetThemeColor(hTheme, WP_CAPTION, CS_ACTIVE,
                  wizard->isActiveWindow() ? TMT_FILLCOLORHINT : TMT_BORDERCOLORHINT, &rgb);
    BYTE r = GetRValue(rgb);
    BYTE g = GetGValue(rgb);
    BYTE b = GetBValue(rgb);
    return QColor(r, g, b);
}

bool QVistaHelper::setDWMTitleBar(TitleBarChangeType type)
{
    MARGINS mar = {0, 0, 0, 0};
    if (type == NormalTitleBar)
        mar.cyTopHeight = 0;
    else
        mar.cyTopHeight = (titleBarSize() + topOffset(wizard)) * QVistaHelper::m_devicePixelRatio;
    if (const HWND wizardHandle = wizardHWND()) {
        if (SUCCEEDED(DwmExtendFrameIntoClientArea(wizardHandle, &mar)))
            return true;
    }
    return false;
}

Q_GUI_EXPORT HICON qt_pixmapToWinHICON(const QPixmap &);

static LOGFONT getCaptionLogFont(HANDLE hTheme)
{
    LOGFONT result = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0 } };

    if (!hTheme || FAILED(GetThemeSysFont(hTheme, TMT_CAPTIONFONT, &result))) {
        NONCLIENTMETRICS ncm;
        ncm.cbSize = sizeof(NONCLIENTMETRICS);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, false);
        result = ncm.lfMessageFont;
    }
    return result;
}

static bool getCaptionQFont(int dpi, QFont *result)
{
    const HANDLE hTheme = OpenThemeData(GetDesktopWindow(), L"WINDOW");
    if (!hTheme)
        return false;
    // Call into QWindowsNativeInterface to convert the LOGFONT into a QFont.
    const LOGFONT logFont = getCaptionLogFont(hTheme);
    *result = QWindowsFontDatabaseBase::LOGFONT_to_QFont(logFont, dpi);
    return true;
}

void QVistaHelper::drawTitleBar(QPainter *painter)
{
    Q_ASSERT(backButton_);
    QPoint origin;
    const bool isWindow = wizard->isWindow();
    const HDC hdc = QVistaHelper::backingStoreDC(wizard, &origin);

    if (isWindow)
        drawBlackRect(QRect(0, 0, wizard->width(),
                            titleBarSize() + topOffset(wizard)), hdc);
    // The button is positioned in QWizardPrivate::handleAeroStyleChange(),
    // all calculation is relative to it.
    const int btnTop = backButton_->mapToParent(QPoint()).y();
    const int btnHeight = backButton_->size().height();
    const int verticalCenter = (btnTop + btnHeight / 2) - 1;

    const QString text = wizard->window()->windowTitle();
    QFont font;
    if (!isWindow || !getCaptionQFont(wizard->logicalDpiY() * wizard->devicePixelRatio(), &font))
        font = QApplication::font("QMdiSubWindowTitleBar");
    const QFontMetrics fontMetrics(font);
    const QRect brect = fontMetrics.boundingRect(text);
    const int glowOffset = glowSize(wizard);
    int textHeight = brect.height() + 2 * glowOffset;
    int textWidth = brect.width() + 2 * glowOffset;

    const int titleLeft = (wizard->layoutDirection() == Qt::LeftToRight
                           ? titleOffset() - glowOffset
                           : wizard->width() - titleOffset() - textWidth + glowOffset);

    const QRect textRectangle(titleLeft, verticalCenter - textHeight / 2, textWidth, textHeight);
    if (isWindow) {
        drawTitleText(painter, text, textRectangle, hdc);
    } else {
        painter->save();
        painter->setFont(font);
        painter->drawText(textRectangle, Qt::AlignVCenter | Qt::AlignHCenter, text);
        painter->restore();
    }

    const QIcon windowIcon = wizard->windowIcon();
    if (!windowIcon.isNull()) {
        const int size = QVistaHelper::iconSize(wizard);
        const int iconLeft = (wizard->layoutDirection() == Qt::LeftToRight
                              ? leftMargin(wizard)
                              : wizard->width() - leftMargin(wizard) - size);

        const QPoint pos(origin.x() + iconLeft, origin.y() + verticalCenter - size / 2);
        const QPoint posDp = pos * QVistaHelper::m_devicePixelRatio;
        const HICON hIcon = qt_pixmapToWinHICON(windowIcon.pixmap(size * QVistaHelper::m_devicePixelRatio));
        DrawIconEx(hdc, posDp.x(), posDp.y(), hIcon, 0, 0, 0, NULL, DI_NORMAL | DI_COMPAT);
        DestroyIcon(hIcon);
    }
}

void QVistaHelper::setTitleBarIconAndCaptionVisible(bool visible)
{
    WTA_OPTIONS opt;
    opt.dwFlags = WTNCA_NODRAWICON | WTNCA_NODRAWCAPTION;
    if (visible)
        opt.dwMask = 0;
    else
        opt.dwMask = WTNCA_NODRAWICON | WTNCA_NODRAWCAPTION;
    if (const HWND handle = wizardHWND())
        SetWindowThemeAttribute(handle, WTA_NONCLIENT, &opt, sizeof(WTA_OPTIONS));
}

bool QVistaHelper::winEvent(MSG* msg, qintptr *result)
{
    switch (msg->message) {
    case WM_NCHITTEST: {
        LRESULT lResult;
        // Perform hit testing using DWM
        if (DwmDefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam, &lResult)) {
            // DWM returned a hit, no further processing necessary
            *result = lResult;
        } else {
            // DWM didn't return a hit, process using DefWindowProc
            lResult = DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
            // If DefWindowProc returns a window caption button, just return HTCLIENT (client area).
            // This avoid unnecessary hits to Windows NT style caption buttons which aren't visible but are
            // located just under the Aero style window close button.
            if (lResult == HTCLOSE || lResult == HTMAXBUTTON || lResult == HTMINBUTTON || lResult == HTHELP)
                *result = HTCLIENT;
            else
                *result = lResult;
        }
        break;
    }
    default:
        LRESULT lResult;
        // Pass to DWM to handle
        if (DwmDefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam, &lResult))
            *result = lResult;
        // If the message wasn't handled by DWM, continue processing it as normal
        else
            return false;
    }

    return true;
}

void QVistaHelper::setMouseCursor(QPoint pos)
{
#ifndef QT_NO_CURSOR
    if (rtTop.contains(pos))
        wizard->setCursor(Qt::SizeVerCursor);
    else
        wizard->setCursor(Qt::ArrowCursor);
#endif
}

void QVistaHelper::mouseEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseMove:
        mouseMoveEvent(static_cast<QMouseEvent *>(event));
        break;
    case QEvent::MouseButtonPress:
        mousePressEvent(static_cast<QMouseEvent *>(event));
        break;
    case QEvent::MouseButtonRelease:
        mouseReleaseEvent(static_cast<QMouseEvent *>(event));
        break;
    default:
        break;
    }
}

bool QVistaHelper::handleWinEvent(MSG *message, qintptr *result)
{
    bool status = false;
    if (wizard->wizardStyle() == QWizard::AeroStyle) {
        status = winEvent(message, result);
        if (message->message == WM_NCPAINT)
            wizard->update();
    }
    return status;
}

void QVistaHelper::resizeEvent(QResizeEvent * event)
{
    Q_UNUSED(event);
    rtTop = QRect (0, 0, wizard->width(), frameSize());
    int height = captionSize() + topOffset(wizard);
    rtTitle = QRect (0, frameSize(), wizard->width(), height);
}

void QVistaHelper::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(wizard);
    drawTitleBar(&painter);
}

void QVistaHelper::mouseMoveEvent(QMouseEvent *event)
{
    if (wizard->windowState() & Qt::WindowMaximized) {
        event->ignore();
        return;
    }

    QRect rect = wizard->geometry();
    if (pressed) {
        switch (change) {
        case resizeTop:
            {
                const int dy = event->pos().y() - pressedPos.y();
                if ((dy > 0 && rect.height() > wizard->minimumHeight())
                    || (dy < 0 && rect.height() < wizard->maximumHeight()))
                    rect.setTop(rect.top() + dy);
            }
            break;
        case movePosition: {
            QPoint newPos = event->pos() - pressedPos;
            rect.moveLeft(rect.left() + newPos.x());
            rect.moveTop(rect.top() + newPos.y());
            break; }
        default:
            break;
        }
        wizard->setGeometry(rect);

    } else {
        setMouseCursor(event->pos());
    }
    event->ignore();
}

void QVistaHelper::mousePressEvent(QMouseEvent *event)
{
    change = noChange;

    if (event->button() != Qt::LeftButton || wizard->windowState() & Qt::WindowMaximized) {
        event->ignore();
        return;
    }

    if (rtTitle.contains(event->pos())) {
        change = movePosition;
    } else if (rtTop.contains(event->pos()))
        change = resizeTop;

    if (change != noChange) {
        setMouseCursor(event->pos());
        pressed = true;
        pressedPos = event->pos();
    } else {
        event->ignore();
    }
}

void QVistaHelper::mouseReleaseEvent(QMouseEvent *event)
{
    change = noChange;
    if (pressed) {
        pressed = false;
        wizard->releaseMouse();
        setMouseCursor(event->pos());
    }
    event->ignore();
}

static inline LPARAM pointToLParam(const QPointF &p, const QWidget *w)
{
    const auto point = QHighDpi::toNativePixels(p, w->screen()).toPoint();
    return MAKELPARAM(point.x(), point.y());
}

bool QVistaHelper::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != wizard)
        return QObject::eventFilter(obj, event);

    if (event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        qintptr result;
        MSG msg;
        msg.message = WM_NCHITTEST;
        msg.wParam  = 0;
        msg.lParam = pointToLParam(mouseEvent->globalPosition(), wizard);
        msg.hwnd = wizardHWND();
        winEvent(&msg, &result);
        msg.wParam = result;
        msg.message = WM_NCMOUSEMOVE;
        winEvent(&msg, &result);
     } else if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        if (mouseEvent->button() == Qt::LeftButton) {
            qintptr result;
            MSG msg;
            msg.message = WM_NCHITTEST;
            msg.wParam  = 0;
            msg.lParam = pointToLParam(mouseEvent->globalPosition(), wizard);
            msg.hwnd = wizardHWND();
            winEvent(&msg, &result);
            msg.wParam = result;
            msg.message = WM_NCLBUTTONDOWN;
            winEvent(&msg, &result);
        }
     } else if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        if (mouseEvent->button() == Qt::LeftButton) {
            qintptr result;
            MSG msg;
            msg.message = WM_NCHITTEST;
            msg.wParam  = 0;
            msg.lParam = pointToLParam(mouseEvent->globalPosition(), wizard);
            msg.hwnd = wizardHWND();
            winEvent(&msg, &result);
            msg.wParam = result;
            msg.message = WM_NCLBUTTONUP;
            winEvent(&msg, &result);
        }
     }

     return false;
}

// Return a HDC for the wizard along with the transformation if the
// wizard is a child window.
HDC QVistaHelper::backingStoreDC(const QWidget *wizard, QPoint *offset)
{
    HDC hdc = static_cast<HDC>(QGuiApplication::platformNativeInterface()->nativeResourceForBackingStore(QByteArrayLiteral("getDC"), wizard->backingStore()));
    *offset = QPoint(0, 0);
    if (!wizard->windowHandle())
        if (QWidget *nativeParent = wizard->nativeParentWidget())
            *offset = wizard->mapTo(nativeParent, *offset);
    return hdc;
}

HWND QVistaHelper::wizardHWND() const
{
    // Obtain the HWND if the wizard is a top-level window.
    // Do not use winId() as this enforces native children of the parent
    // widget when called before show() as happens when calling setWizardStyle().
    if (QWindow *window = wizard->windowHandle())
        if (window->handle())
            if (void *vHwnd = QGuiApplication::platformNativeInterface()->nativeResourceForWindow(QByteArrayLiteral("handle"), window))
                return static_cast<HWND>(vHwnd);
    qWarning().nospace() << "Failed to obtain HWND for wizard.";
    return 0;
}

void QVistaHelper::drawTitleText(QPainter *painter, const QString &text, const QRect &rect, HDC hdc)
{
    Q_UNUSED(painter);

    const QRect rectDp = QRect(rect.topLeft() * QVistaHelper::m_devicePixelRatio,
                               rect.size() * QVistaHelper::m_devicePixelRatio);
    const HANDLE hTheme = OpenThemeData(GetDesktopWindow(), L"WINDOW");
    if (!hTheme)
        return;
    // Set up a memory DC and bitmap that we'll draw into
    HDC dcMem;
    HBITMAP bmp;
    BITMAPINFO dib;
    ZeroMemory(&dib, sizeof(dib));
    dcMem = CreateCompatibleDC(hdc);

    dib.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    dib.bmiHeader.biWidth = rectDp.width();
    dib.bmiHeader.biHeight = -rectDp.height();
    dib.bmiHeader.biPlanes = 1;
    dib.bmiHeader.biBitCount = 32;
    dib.bmiHeader.biCompression = BI_RGB;

    bmp = CreateDIBSection(hdc, &dib, DIB_RGB_COLORS, NULL, NULL, 0);

    // Set up the DC
    const LOGFONT captionLogFont = getCaptionLogFont(hTheme);
    const HFONT hCaptionFont = CreateFontIndirect(&captionLogFont);
    auto hOldBmp = reinterpret_cast<HBITMAP>(SelectObject(dcMem, (HGDIOBJ) bmp));
    auto hOldFont = reinterpret_cast<HFONT>(SelectObject(dcMem, (HGDIOBJ) hCaptionFont));

    // Draw the text!
    DTTOPTS dto;
    memset(&dto, 0, sizeof(dto));
    dto.dwSize = sizeof(dto);
    const UINT uFormat = DT_SINGLELINE|DT_CENTER|DT_VCENTER|DT_NOPREFIX;
    RECT rctext ={0,0, rectDp.width(), rectDp.height()};

    dto.dwFlags = DTT_COMPOSITED|DTT_GLOWSIZE;
    dto.iGlowSize = glowSize(wizard);

    DrawThemeTextEx(hTheme, dcMem, 0, 0, reinterpret_cast<LPCWSTR>(text.utf16()), -1, uFormat, &rctext, &dto );
    BitBlt(hdc, rectDp.left(), rectDp.top(), rectDp.width(), rectDp.height(), dcMem, 0, 0, SRCCOPY);
    SelectObject(dcMem, (HGDIOBJ) hOldBmp);
    SelectObject(dcMem, (HGDIOBJ) hOldFont);
    DeleteObject(bmp);
    DeleteObject(hCaptionFont);
    DeleteDC(dcMem);
    //ReleaseDC(hwnd, hdc);
}

void QVistaHelper::drawBlackRect(const QRect &rect, HDC hdc)
{
    // Set up a memory DC and bitmap that we'll draw into
    const QRect rectDp = QRect(rect.topLeft() * QVistaHelper::m_devicePixelRatio,
                                   rect.size() * QVistaHelper::m_devicePixelRatio);
    HDC dcMem;
    HBITMAP bmp;
    BITMAPINFO dib;
    ZeroMemory(&dib, sizeof(dib));
    dcMem = CreateCompatibleDC(hdc);

    dib.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    dib.bmiHeader.biWidth = rectDp.width();
    dib.bmiHeader.biHeight = -rectDp.height();
    dib.bmiHeader.biPlanes = 1;
    dib.bmiHeader.biBitCount = 32;
    dib.bmiHeader.biCompression = BI_RGB;

    bmp = CreateDIBSection(hdc, &dib, DIB_RGB_COLORS, NULL, NULL, 0);
    auto hOldBmp = reinterpret_cast<HBITMAP>(SelectObject(dcMem, (HGDIOBJ) bmp));

    BitBlt(hdc, rectDp.left(), rectDp.top(), rectDp.width(), rectDp.height(), dcMem, 0, 0, SRCCOPY);
    SelectObject(dcMem, (HGDIOBJ) hOldBmp);

    DeleteObject(bmp);
    DeleteDC(dcMem);
}

int QVistaHelper::frameSizeDp()
{
    return GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
}

int QVistaHelper::captionSizeDp()
{
    return GetSystemMetrics(SM_CYCAPTION);
}

int QVistaHelper::titleOffset()
{
    int iconOffset = wizard ->windowIcon().isNull() ? 0 : iconSize(wizard) + iconSpacing;
    return leftMargin(wizard) + iconOffset;
}

int QVistaHelper::iconSize(const QPaintDevice *device)
{
    return QStyleHelper::dpiScaled(16, device); // Standard Aero
}

int QVistaHelper::glowSize(const QPaintDevice *device)
{
    return QStyleHelper::dpiScaled(10, device);
}

int QVistaHelper::topOffset(const QPaintDevice *device)
{
    static const int aeroOffset = QStyleHelper::dpiScaled(13, device);
    return aeroOffset + titleBarSize();
}

QT_END_NAMESPACE

#endif // style_windowsvista
