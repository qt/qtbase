/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QT_NO_WIZARD
#ifndef QT_NO_STYLE_WINDOWSVISTA

#include "qwizard_win_p.h"
#include <private/qapplication_p.h>
#include <qpa/qplatformnativeinterface.h>
#include "qwizard.h"
#include "qpaintengine.h"
#include "qapplication.h"
#include <QtCore/QVariant>
#include <QtCore/QDebug>
#include <QtGui/QMouseEvent>
#include <QtGui/QWindow>
#include <QtWidgets/QDesktopWidget>

#include <uxtheme.h>
#include <vssym32.h>
#include <dwmapi.h>

Q_DECLARE_METATYPE(QMargins)

#ifndef WM_DWMCOMPOSITIONCHANGED
#  define WM_DWMCOMPOSITIONCHANGED 0x031E
#endif

QT_BEGIN_NAMESPACE

int QVistaHelper::instanceCount = 0;
int QVistaHelper::m_devicePixelRatio = 1;
QVistaHelper::VistaState QVistaHelper::cachedVistaState = QVistaHelper::Dirty;

/******************************************************************************
** QVistaBackButton
*/

QVistaBackButton::QVistaBackButton(QWidget *widget)
    : QAbstractButton(widget)
{
    setFocusPolicy(Qt::NoFocus);
    // Native dialogs use ALT-Left even in RTL mode, so do the same, even if it might be counter-intuitive.
    setShortcut(QKeySequence(Qt::ALT | Qt::Key_Left));
}

QSize QVistaBackButton::sizeHint() const
{
    ensurePolished();
    int size = int(QStyleHelper::dpiScaled(32));
    int width = size, height = size;
    return QSize(width, height);
}

void QVistaBackButton::enterEvent(QEvent *event)
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
    const int dpr = devicePixelRatio();
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
    if (instanceCount++ == 0)
        cachedVistaState = Dirty;
    backButton_ = new QVistaBackButton(wizard);
    backButton_->hide();

    // Handle diff between Windows 7 and Vista
    iconSpacing = QStyleHelper::dpiScaled(7);
    textSpacing = QSysInfo::WindowsVersion >= QSysInfo::WV_WINDOWS7 ?
                  iconSpacing : QStyleHelper::dpiScaled(20);
}

QVistaHelper::~QVistaHelper()
{
    --instanceCount;
}

void QVistaHelper::updateCustomMargins(bool vistaMargins)
{
    if (QWindow *window = wizard->windowHandle()) {
        // Reduce top frame to zero since we paint it ourselves. Use
        // device pixel to avoid rounding errors.
        const QMargins customMarginsDp = vistaMargins
            ? QMargins(0, -titleBarSizeDp(), 0, 0)
            : QMargins();
        const QVariant customMarginsV = qVariantFromValue(customMarginsDp);
        // The dynamic property takes effect when creating the platform window.
        window->setProperty("_q_windowsCustomMargins", customMarginsV);
        // If a platform window exists, change via native interface.
        if (QPlatformWindow *platformWindow = window->handle()) {
            QGuiApplication::platformNativeInterface()->
                setWindowProperty(platformWindow, QStringLiteral("WindowsCustomMargins"),
                                  customMarginsV);
        }
    }
}

bool QVistaHelper::isCompositionEnabled()
{
    BOOL bEnabled;
    return SUCCEEDED(DwmIsCompositionEnabled(&bEnabled)) && bEnabled;
}

bool QVistaHelper::isThemeActive()
{
    return IsThemeActive();
}

QVistaHelper::VistaState QVistaHelper::vistaState()
{
    if (instanceCount == 0 || cachedVistaState == Dirty)
        cachedVistaState =
            isCompositionEnabled() ? VistaAero : isThemeActive() ? VistaBasic : Classic;
    return cachedVistaState;
}

void QVistaHelper::disconnectBackButton()
{
    if (backButton_) // Leave QStyleSheetStyle's connections on destroyed() intact.
        backButton_->disconnect(SIGNAL(clicked()));
}

QColor QVistaHelper::basicWindowFrameColor()
{
    DWORD rgb;
    HWND handle = QApplicationPrivate::getHWNDForWidget(QApplication::desktop());
    const HANDLE hTheme = OpenThemeData(handle, L"WINDOW");
    GetThemeColor(hTheme, WP_CAPTION, CS_ACTIVE,
                  wizard->isActiveWindow() ? TMT_FILLCOLORHINT : TMT_BORDERCOLORHINT, &rgb);
    BYTE r = GetRValue(rgb);
    BYTE g = GetGValue(rgb);
    BYTE b = GetBValue(rgb);
    return QColor(r, g, b);
}

bool QVistaHelper::setDWMTitleBar(TitleBarChangeType type)
{
    bool value = false;
    if (vistaState() == VistaAero) {
        MARGINS mar = {0, 0, 0, 0};
        if (type == NormalTitleBar)
            mar.cyTopHeight = 0;
        else
            mar.cyTopHeight = (titleBarSize() + topOffset()) * QVistaHelper::m_devicePixelRatio;
        if (const HWND wizardHandle = wizardHWND())
            if (SUCCEEDED(DwmExtendFrameIntoClientArea(wizardHandle, &mar)))
                value = true;
    }
    return value;
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
    const HANDLE hTheme =
        OpenThemeData(QApplicationPrivate::getHWNDForWidget(QApplication::desktop()), L"WINDOW");
    if (!hTheme)
        return false;
    // Call into QWindowsNativeInterface to convert the LOGFONT into a QFont.
    const LOGFONT logFont = getCaptionLogFont(hTheme);
    QPlatformNativeInterface *ni = QGuiApplication::platformNativeInterface();
    return ni && QMetaObject::invokeMethod(ni, "logFontToQFont", Qt::DirectConnection,
                                           Q_RETURN_ARG(QFont, *result),
                                           Q_ARG(const void *, &logFont),
                                           Q_ARG(int, dpi));
}

void QVistaHelper::drawTitleBar(QPainter *painter)
{
    Q_ASSERT(backButton_);
    QPoint origin;
    const bool isWindow = wizard->isWindow();
    const HDC hdc = QVistaHelper::backingStoreDC(wizard, &origin);

    if (vistaState() == VistaAero && isWindow)
        drawBlackRect(QRect(0, 0, wizard->width(),
                            titleBarSize() + topOffset()), hdc);
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
    int textHeight = brect.height();
    int textWidth = brect.width();
    int glowOffset = 0;

    if (vistaState() == VistaAero) {
        textHeight += 2 * glowSize();
        textWidth += 2 * glowSize();
        glowOffset = glowSize();
    }

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
        const int size = QVistaHelper::iconSize();
        const int iconLeft = (wizard->layoutDirection() == Qt::LeftToRight
                              ? leftMargin()
                              : wizard->width() - leftMargin() - size);

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

bool QVistaHelper::winEvent(MSG* msg, long* result)
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

bool QVistaHelper::handleWinEvent(MSG *message, long *result)
{
    if (message->message == WM_THEMECHANGED || message->message == WM_DWMCOMPOSITIONCHANGED)
        cachedVistaState = Dirty;

    bool status = false;
    if (wizard->wizardStyle() == QWizard::AeroStyle && vistaState() == VistaAero) {
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
    int height = captionSize() + topOffset();
    if (vistaState() == VistaBasic)
        height -= titleBarSize();
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

    } else if (vistaState() == VistaAero) {
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
        change = (vistaState() == VistaAero) ? resizeTop : movePosition;

    if (change != noChange) {
        if (vistaState() == VistaAero)
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
        if (vistaState() == VistaAero)
            setMouseCursor(event->pos());
    }
    event->ignore();
}

bool QVistaHelper::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != wizard)
        return QObject::eventFilter(obj, event);

    if (event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        long result;
        MSG msg;
        msg.message = WM_NCHITTEST;
        msg.wParam  = 0;
        msg.lParam = MAKELPARAM(mouseEvent->globalX(), mouseEvent->globalY());
        msg.hwnd = wizardHWND();
        winEvent(&msg, &result);
        msg.wParam = result;
        msg.message = WM_NCMOUSEMOVE;
        winEvent(&msg, &result);
     } else if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        if (mouseEvent->button() == Qt::LeftButton) {
            long result;
            MSG msg;
            msg.message = WM_NCHITTEST;
            msg.wParam  = 0;
            msg.lParam = MAKELPARAM(mouseEvent->globalX(), mouseEvent->globalY());
            msg.hwnd = wizardHWND();
            winEvent(&msg, &result);
            msg.wParam = result;
            msg.message = WM_NCLBUTTONDOWN;
            winEvent(&msg, &result);
        }
     } else if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        if (mouseEvent->button() == Qt::LeftButton) {
            long result;
            MSG msg;
            msg.message = WM_NCHITTEST;
            msg.wParam  = 0;
            msg.lParam = MAKELPARAM(mouseEvent->globalX(), mouseEvent->globalY());
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

bool QVistaHelper::drawTitleText(QPainter *painter, const QString &text, const QRect &rect, HDC hdc)
{
    bool value = false;
    if (vistaState() == VistaAero) {
        const QRect rectDp = QRect(rect.topLeft() * QVistaHelper::m_devicePixelRatio,
                                   rect.size() * QVistaHelper::m_devicePixelRatio);
        HWND handle = QApplicationPrivate::getHWNDForWidget(QApplication::desktop());
        const HANDLE hTheme = OpenThemeData(handle, L"WINDOW");
        if (!hTheme) return false;
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
        HBITMAP hOldBmp = (HBITMAP)SelectObject(dcMem, (HGDIOBJ) bmp);
        HFONT hOldFont = (HFONT)SelectObject(dcMem, (HGDIOBJ) hCaptionFont);

        // Draw the text!
        DTTOPTS dto;
        memset(&dto, 0, sizeof(dto));
        dto.dwSize = sizeof(dto);
        const UINT uFormat = DT_SINGLELINE|DT_CENTER|DT_VCENTER|DT_NOPREFIX;
        RECT rctext ={0,0, rectDp.width(), rectDp.height()};

        dto.dwFlags = DTT_COMPOSITED|DTT_GLOWSIZE;
        dto.iGlowSize = glowSize();

        DrawThemeTextEx(hTheme, dcMem, 0, 0, reinterpret_cast<LPCWSTR>(text.utf16()), -1, uFormat, &rctext, &dto );
        BitBlt(hdc, rectDp.left(), rectDp.top(), rectDp.width(), rectDp.height(), dcMem, 0, 0, SRCCOPY);
        SelectObject(dcMem, (HGDIOBJ) hOldBmp);
        SelectObject(dcMem, (HGDIOBJ) hOldFont);
        DeleteObject(bmp);
        DeleteObject(hCaptionFont);
        DeleteDC(dcMem);
        //ReleaseDC(hwnd, hdc);
    } else if (vistaState() == VistaBasic) {
        painter->drawText(rect, text);
    }
    return value;
}

bool QVistaHelper::drawBlackRect(const QRect &rect, HDC hdc)
{
    bool value = false;
    if (vistaState() == VistaAero) {
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
        HBITMAP hOldBmp = (HBITMAP)SelectObject(dcMem, (HGDIOBJ) bmp);

        BitBlt(hdc, rectDp.left(), rectDp.top(), rectDp.width(), rectDp.height(), dcMem, 0, 0, SRCCOPY);
        SelectObject(dcMem, (HGDIOBJ) hOldBmp);

        DeleteObject(bmp);
        DeleteDC(dcMem);
    }
    return value;
}

#if !defined(_MSC_VER) || _MSC_VER < 1700
static inline int getWindowBottomMargin()
{
    return GetSystemMetrics(SM_CYSIZEFRAME);
}
#else // !_MSC_VER || _MSC_VER < 1700
// QTBUG-36192, GetSystemMetrics(SM_CYSIZEFRAME) returns bogus values
// for MSVC2012 which leads to the custom margin having no effect since
// that only works when removing the entire margin.
static inline int getWindowBottomMargin()
{
    RECT rect = {0, 0, 0, 0};
    AdjustWindowRectEx(&rect, WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_THICKFRAME | WS_DLGFRAME, FALSE, 0);
    return qAbs(rect.bottom);
}
#endif // _MSC_VER >= 1700

int QVistaHelper::frameSizeDp()
{
    return getWindowBottomMargin();
}

int QVistaHelper::captionSizeDp()
{
    return GetSystemMetrics(SM_CYCAPTION);
}

int QVistaHelper::titleOffset()
{
    int iconOffset = wizard ->windowIcon().isNull() ? 0 : iconSize() + textSpacing;
    return leftMargin() + iconOffset;
}

int QVistaHelper::iconSize()
{
    return QStyleHelper::dpiScaled(16); // Standard Aero
}

int QVistaHelper::glowSize()
{
    return QStyleHelper::dpiScaled(10);
}

int QVistaHelper::topOffset()
{
    if (vistaState() != VistaAero)
        return titleBarSize() + 3;
    static const int aeroOffset =
        QSysInfo::WindowsVersion == QSysInfo::WV_WINDOWS7 ?
        QStyleHelper::dpiScaled(4) : QStyleHelper::dpiScaled(13);
    return aeroOffset + titleBarSize();
}

QT_END_NAMESPACE

#endif // QT_NO_STYLE_WINDOWSVISTA

#endif // QT_NO_WIZARD
