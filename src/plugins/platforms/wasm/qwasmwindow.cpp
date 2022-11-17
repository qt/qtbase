// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qpa/qwindowsysteminterface.h>
#include <private/qguiapplication_p.h>
#include <QtGui/private/qopenglcontext_p.h>
#include <QtGui/private/qwindow_p.h>
#include <QtGui/qopenglcontext.h>
#include <private/qpixmapcache_p.h>

#include "qwasmwindow.h"
#include "qwasmscreen.h"
#include "qwasmstylepixmaps_p.h"
#include "qwasmcompositor.h"
#include "qwasmeventdispatcher.h"

#include <iostream>


QT_BEGIN_NAMESPACE

Q_GUI_EXPORT int qt_defaultDpiX();

namespace {
// from commonstyle.cpp
static QPixmap cachedPixmapFromXPM(const char *const *xpm)
{
    QPixmap result;
    const QString tag = QString::asprintf("xpm:0x%p", static_cast<const void *>(xpm));
    if (!QPixmapCache::find(tag, &result)) {
        result = QPixmap(xpm);
        QPixmapCache::insert(tag, result);
    }
    return result;
}

QPalette makePalette()
{
    QPalette palette;
    palette.setColor(QPalette::Active, QPalette::Highlight,
                     palette.color(QPalette::Active, QPalette::Highlight));
    palette.setColor(QPalette::Active, QPalette::Base,
                     palette.color(QPalette::Active, QPalette::Highlight));
    palette.setColor(QPalette::Inactive, QPalette::Highlight,
                     palette.color(QPalette::Inactive, QPalette::Dark));
    palette.setColor(QPalette::Inactive, QPalette::Base,
                     palette.color(QPalette::Inactive, QPalette::Dark));
    palette.setColor(QPalette::Inactive, QPalette::HighlightedText,
                     palette.color(QPalette::Inactive, QPalette::Window));

    return palette;
}

void drawItemPixmap(QPainter *painter, const QRect &rect, int alignment, const QPixmap &pixmap)
{
    qreal scale = pixmap.devicePixelRatio();
    QSize size = pixmap.size() / scale;
    int x = rect.x();
    int y = rect.y();
    int w = size.width();
    int h = size.height();
    if ((alignment & Qt::AlignVCenter) == Qt::AlignVCenter)
        y += rect.size().height() / 2 - h / 2;
    else if ((alignment & Qt::AlignBottom) == Qt::AlignBottom)
        y += rect.size().height() - h;
    if ((alignment & Qt::AlignRight) == Qt::AlignRight)
        x += rect.size().width() - w;
    else if ((alignment & Qt::AlignHCenter) == Qt::AlignHCenter)
        x += rect.size().width() / 2 - w / 2;

    QRect aligned = QRect(x, y, w, h);
    QRect inter = aligned.intersected(rect);

    painter->drawPixmap(inter.x(), inter.y(), pixmap, inter.x() - aligned.x(),
                        inter.y() - aligned.y(), inter.width() * scale, inter.height() * scale);
}
}

QWasmWindow::QWasmWindow(QWindow *w, QWasmCompositor *compositor, QWasmBackingStore *backingStore)
    : QPlatformWindow(w),
      m_window(w),
      m_compositor(compositor),
      m_backingStore(backingStore)
{
    m_needsCompositor = w->surfaceType() != QSurface::OpenGLSurface;
    static int serialNo = 0;
    m_winid = ++serialNo;

    m_compositor->addWindow(this);

    // Pure OpenGL windows draw directly using egl, disable the compositor.
    m_compositor->setEnabled(w->surfaceType() != QSurface::OpenGLSurface);
}

QWasmWindow::~QWasmWindow()
{
    m_compositor->removeWindow(this);
    if (m_requestAnimationFrameId > -1)
        emscripten_cancel_animation_frame(m_requestAnimationFrameId);
}

void QWasmWindow::destroy()
{
    if (m_backingStore)
        m_backingStore->destroy();
}

void QWasmWindow::initialize()
{
    QRect rect = windowGeometry();

    constexpr int minSizeBoundForDialogsAndRegularWindows = 100;
    const int windowType = window()->flags() & Qt::WindowType_Mask;
    const int systemMinSizeLowerBound = windowType == Qt::Window || windowType == Qt::Dialog
            ? minSizeBoundForDialogsAndRegularWindows
            : 0;

    const QSize minimumSize(std::max(windowMinimumSize().width(), systemMinSizeLowerBound),
                            std::max(windowMinimumSize().height(), systemMinSizeLowerBound));
    const QSize maximumSize = windowMaximumSize();
    const QSize targetSize = !rect.isEmpty() ? rect.size() : minimumSize;

    rect.setWidth(qBound(minimumSize.width(), targetSize.width(), maximumSize.width()));
    rect.setHeight(qBound(minimumSize.width(), targetSize.height(), maximumSize.height()));

    setWindowState(window()->windowStates());
    setWindowFlags(window()->flags());
    setWindowTitle(window()->title());
    if (window()->isTopLevel())
        setWindowIcon(window()->icon());
    m_normalGeometry = rect;
    QPlatformWindow::setGeometry(m_normalGeometry);
}

QWasmScreen *QWasmWindow::platformScreen() const
{
    return static_cast<QWasmScreen *>(window()->screen()->handle());
}

void QWasmWindow::setGeometry(const QRect &rect)
{
    const QRect clientAreaRect = ([this, &rect]() {
        if (!m_needsCompositor)
            return rect;

        const int captionHeight = window()->geometry().top() - window()->frameGeometry().top();
        const auto screenGeometry = screen()->geometry();

        QRect result(rect);
        result.moveTop(std::max(std::min(rect.y(), screenGeometry.bottom()),
                                screenGeometry.y() + captionHeight));
        return result;
    })();
    bool shouldInvalidate = true;
    if (!m_windowState.testFlag(Qt::WindowFullScreen)
        && !m_windowState.testFlag(Qt::WindowMaximized)) {
        shouldInvalidate = m_normalGeometry.size() != clientAreaRect.size();
        m_normalGeometry = clientAreaRect;
    }
    QWindowSystemInterface::handleGeometryChange(window(), clientAreaRect);
    if (shouldInvalidate)
        invalidate();
    else
        m_compositor->requestUpdateWindow(this);
}

void QWasmWindow::setVisible(bool visible)
{
    if (visible)
        applyWindowState();
    m_compositor->setVisible(this, visible);
}

bool QWasmWindow::isVisible()
{
    return window()->isVisible();
}

QMargins QWasmWindow::frameMargins() const
{
    int border = hasTitleBar() ? 4. * (qreal(qt_defaultDpiX()) / 96.0) : 0;
    int titleBarHeight = hasTitleBar() ? titleHeight() : 0;

    QMargins margins;
    margins.setLeft(border);
    margins.setRight(border);
    margins.setTop(2*border + titleBarHeight);
    margins.setBottom(border);

    return margins;
}

void QWasmWindow::raise()
{
    m_compositor->raise(this);
    invalidate();
}

void QWasmWindow::lower()
{
    m_compositor->lower(this);
    invalidate();
}

WId QWasmWindow::winId() const
{
    return m_winid;
}

void QWasmWindow::propagateSizeHints()
{
    QRect rect = windowGeometry();
    if (rect.size().width() < windowMinimumSize().width()
        && rect.size().height() < windowMinimumSize().height()) {
        rect.setSize(windowMinimumSize());
        setGeometry(rect);
    }
}

bool QWasmWindow::startSystemResize(Qt::Edges edges)
{
    m_compositor->startResize(edges);

    return true;
}

void QWasmWindow::injectMousePressed(const QPoint &local, const QPoint &global,
                                      Qt::MouseButton button, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(local);
    Q_UNUSED(mods);

    if (!hasTitleBar() || button != Qt::LeftButton)
        return;

    if (const auto controlHit = titleBarHitTest(global))
        m_activeControl = *controlHit;

    invalidate();
}

void QWasmWindow::injectMouseReleased(const QPoint &local, const QPoint &global,
                                       Qt::MouseButton button, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(local);
    Q_UNUSED(mods);

    if (!hasTitleBar() || button != Qt::LeftButton)
        return;

    if (const auto controlHit = titleBarHitTest(global)) {
        if (m_activeControl == *controlHit) {
            switch (*controlHit) {
            case SC_TitleBarCloseButton:
                window()->close();
                break;
            case SC_TitleBarMaxButton:
                window()->setWindowState(Qt::WindowMaximized);
                break;
            case SC_TitleBarNormalButton:
                window()->setWindowState(Qt::WindowNoState);
                break;
            case SC_None:
            case SC_TitleBarLabel:
            case SC_TitleBarSysMenu:
                Q_ASSERT(false); // These types are not clickable
                return;
            }
        }
    }

    m_activeControl = SC_None;

    invalidate();
}

int QWasmWindow::titleHeight() const
{
    return 18. * (qreal(qt_defaultDpiX()) / 96.0);//dpiScaled(18.);
}

int QWasmWindow::borderWidth() const
{
    return  4. * (qreal(qt_defaultDpiX()) / 96.0);// dpiScaled(4.);
}

QRegion QWasmWindow::resizeRegion() const
{
    int border = borderWidth();
    QRegion result(window()->frameGeometry().adjusted(-border, -border, border, border));
    result -= window()->frameGeometry().adjusted(border, border, -border, -border);

    return result;
}

bool QWasmWindow::isPointOnTitle(QPoint globalPoint) const
{
    const auto pointInFrameCoords = globalPoint - windowFrameGeometry().topLeft();
    if (const auto titleRect =
                getTitleBarControlRect(makeTitleBarOptions(), TitleBarControl::SC_TitleBarLabel)) {
        return titleRect->contains(pointInFrameCoords);
    }
    return false;
}

bool QWasmWindow::isPointOnResizeRegion(QPoint point) const
{
    // Certain windows, like undocked dock widgets, are both popups and dialogs. Those should be
    // resizable.
    if (windowIsPopupType(window()->flags()))
        return false;
    return (window()->maximumSize().isEmpty() || window()->minimumSize() != window()->maximumSize())
            && resizeRegion().contains(point);
}

Qt::Edges QWasmWindow::resizeEdgesAtPoint(QPoint point) const
{
    const QPoint topLeft = window()->frameGeometry().topLeft() - QPoint(5, 5);
    const QPoint bottomRight = window()->frameGeometry().bottomRight() + QPoint(5, 5);
    const int gripAreaWidth = std::min(20, (bottomRight.y() - topLeft.y()) / 2);

    const QRect top(topLeft, QPoint(bottomRight.x(), topLeft.y() + gripAreaWidth));
    const QRect bottom(QPoint(topLeft.x(), bottomRight.y() - gripAreaWidth), bottomRight);
    const QRect left(topLeft, QPoint(topLeft.x() + gripAreaWidth, bottomRight.y()));
    const QRect right(QPoint(bottomRight.x() - gripAreaWidth, topLeft.y()), bottomRight);

    Q_ASSERT(!top.intersects(bottom));
    Q_ASSERT(!left.intersects(right));

    Qt::Edges edges(top.contains(point) ? Qt::Edge::TopEdge : Qt::Edge(0));
    edges |= bottom.contains(point) ? Qt::Edge::BottomEdge : Qt::Edge(0);
    edges |= left.contains(point) ? Qt::Edge::LeftEdge : Qt::Edge(0);
    return edges | (right.contains(point) ? Qt::Edge::RightEdge : Qt::Edge(0));
}

std::optional<QRect> QWasmWindow::getTitleBarControlRect(const TitleBarOptions &tb,
                                                         TitleBarControl control) const
{
    const auto leftToRightRect = getTitleBarControlRectLeftToRight(tb, control);
    if (!leftToRightRect)
        return std::nullopt;
    return qApp->layoutDirection() == Qt::LeftToRight
            ? leftToRightRect
            : leftToRightRect->translated(2 * (tb.rect.right() - leftToRightRect->right())
                                                  + leftToRightRect->width() - tb.rect.width(),
                                          0);
}

bool QWasmWindow::TitleBarOptions::hasControl(TitleBarControl control) const
{
    return subControls.testFlag(control);
}

std::optional<QRect> QWasmWindow::getTitleBarControlRectLeftToRight(const TitleBarOptions &tb,
                                                                    TitleBarControl control) const
{
    if (!tb.hasControl(control))
        return std::nullopt;

    const int controlMargin = 2;
    const int controlHeight = tb.rect.height() - controlMargin * 2;
    const int controlWidth = controlHeight;
    const int delta = controlWidth + controlMargin;
    int offsetRight = 0;

    switch (control) {
    case SC_TitleBarLabel: {
        const int leftOffset = tb.hasControl(SC_TitleBarSysMenu) ? delta : 0;
        const int rightOffset = (tb.hasControl(SC_TitleBarCloseButton) ? delta : 0)
                + ((tb.hasControl(SC_TitleBarMaxButton) || tb.hasControl(SC_TitleBarNormalButton))
                           ? delta
                           : 0);

        return tb.rect.adjusted(leftOffset, 0, -rightOffset, 0);
    }
    case SC_TitleBarSysMenu:
        return QRect(tb.rect.left() + controlMargin, tb.rect.top() + controlMargin, controlWidth,
                     controlHeight);
    case SC_TitleBarCloseButton:
        offsetRight = delta;
        break;
    case SC_TitleBarMaxButton:
    case SC_TitleBarNormalButton:
        offsetRight = delta + (tb.hasControl(SC_TitleBarCloseButton) ? delta : 0);
        break;
    case SC_None:
        Q_ASSERT(false);
        break;
    };

    return QRect(tb.rect.right() - offsetRight, tb.rect.top() + controlMargin, controlWidth,
                 controlHeight);
}

void QWasmWindow::invalidate()
{
    m_compositor->requestUpdateWindow(this);
}

QWasmWindow::TitleBarControl QWasmWindow::activeTitleBarControl() const
{
    return m_activeControl;
}

std::optional<QWasmWindow::TitleBarControl>
QWasmWindow::titleBarHitTest(const QPoint &globalPoint) const
{
    const auto pointInFrameCoords = globalPoint - windowFrameGeometry().topLeft();
    const auto options = makeTitleBarOptions();

    static constexpr TitleBarControl Controls[] = { SC_TitleBarMaxButton, SC_TitleBarCloseButton,
                                                    SC_TitleBarNormalButton };
    auto found = std::find_if(std::begin(Controls), std::end(Controls),
                              [this, &pointInFrameCoords, &options](TitleBarControl control) {
                                  auto controlRect = getTitleBarControlRect(options, control);
                                  return controlRect && controlRect->contains(pointInFrameCoords);
                              });
    return found != std::end(Controls) ? *found : std::optional<TitleBarControl>();
}

void QWasmWindow::setWindowState(Qt::WindowStates newState)
{
    const Qt::WindowStates oldState = m_windowState;
    bool isActive = oldState.testFlag(Qt::WindowActive);

    if (newState.testFlag(Qt::WindowMinimized)) {
        newState.setFlag(Qt::WindowMinimized, false);
        qWarning("Qt::WindowMinimized is not implemented in wasm");
    }

    // Always keep OpenGL apps fullscreen
    if (!m_needsCompositor && !newState.testFlag(Qt::WindowFullScreen)) {
        newState.setFlag(Qt::WindowFullScreen, true);
        qWarning("Qt::WindowFullScreen must be set for OpenGL surfaces");
    }

    // Ignore WindowActive flag in comparison, as we want to preserve it either way
    if ((newState & ~Qt::WindowActive) == (oldState & ~Qt::WindowActive))
        return;

    newState.setFlag(Qt::WindowActive, isActive);

    m_previousWindowState = oldState;
    m_windowState = newState;

    if (isVisible()) {
        applyWindowState();
    }
}

void QWasmWindow::applyWindowState()
{
    QRect newGeom;

    if (m_windowState.testFlag(Qt::WindowFullScreen))
        newGeom = platformScreen()->geometry();
    else if (m_windowState.testFlag(Qt::WindowMaximized))
        newGeom = platformScreen()->availableGeometry();
    else
        newGeom = normalGeometry();

    QWindowSystemInterface::handleWindowStateChanged(window(), m_windowState, m_previousWindowState);
    setGeometry(newGeom);
}

void QWasmWindow::drawTitleBar(QPainter *painter) const
{
    const auto tb = makeTitleBarOptions();
    if (const auto ir = getTitleBarControlRect(tb, SC_TitleBarLabel)) {
        QColor left = tb.palette.highlight().color();
        QColor right = tb.palette.base().color();

        QBrush fillBrush(left);
        if (left != right) {
            QPoint p1(tb.rect.x(), tb.rect.top() + tb.rect.height() / 2);
            QPoint p2(tb.rect.right(), tb.rect.top() + tb.rect.height() / 2);
            QLinearGradient lg(p1, p2);
            lg.setColorAt(0, left);
            lg.setColorAt(1, right);
            fillBrush = lg;
        }

        painter->fillRect(tb.rect, fillBrush);
        painter->setPen(tb.palette.highlightedText().color());
        painter->drawText(ir->x() + 2, ir->y(), ir->width() - 2, ir->height(),
                          Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
                          tb.titleBarOptionsString);
    }

    if (const auto ir = getTitleBarControlRect(tb, SC_TitleBarCloseButton)) {
        drawItemPixmap(painter, *ir, Qt::AlignCenter,
                       cachedPixmapFromXPM(qt_close_xpm).scaled(QSize(10, 10)));
    }

    if (const auto ir = getTitleBarControlRect(tb, SC_TitleBarMaxButton)) {
        drawItemPixmap(painter, *ir, Qt::AlignCenter,
                       cachedPixmapFromXPM(qt_maximize_xpm).scaled(QSize(10, 10)));
    }

    if (const auto ir = getTitleBarControlRect(tb, SC_TitleBarNormalButton)) {
        drawItemPixmap(painter, *ir, Qt::AlignCenter,
                       cachedPixmapFromXPM(qt_normalizeup_xpm).scaled(QSize(10, 10)));
    }

    if (const auto ir = getTitleBarControlRect(tb, SC_TitleBarSysMenu)) {
        if (!tb.windowIcon.isNull()) {
            tb.windowIcon.paint(painter, *ir, Qt::AlignCenter);
        } else {
            drawItemPixmap(painter, *ir, Qt::AlignCenter,
                           cachedPixmapFromXPM(qt_menu_xpm).scaled(QSize(10, 10)));
        }
    }
}

QWasmWindow::TitleBarOptions QWasmWindow::makeTitleBarOptions() const
{
    int width = windowFrameGeometry().width();
    int border = borderWidth();

    TitleBarOptions titleBarOptions;

    titleBarOptions.rect = QRect(border, border, width - 2 * border, titleHeight());
    titleBarOptions.flags = window()->flags();
    titleBarOptions.state = window()->windowState();

    bool isMaximized =
            titleBarOptions.state & Qt::WindowMaximized; // this gets reset when maximized

    if (titleBarOptions.flags & (Qt::WindowTitleHint))
        titleBarOptions.subControls |= SC_TitleBarLabel;
    if (titleBarOptions.flags & Qt::WindowMaximizeButtonHint) {
        if (isMaximized)
            titleBarOptions.subControls |= SC_TitleBarNormalButton;
        else
            titleBarOptions.subControls |= SC_TitleBarMaxButton;
    }
    if (titleBarOptions.flags & Qt::WindowSystemMenuHint) {
        titleBarOptions.subControls |= SC_TitleBarCloseButton;
        titleBarOptions.subControls |= SC_TitleBarSysMenu;
    }

    titleBarOptions.palette = makePalette();

    titleBarOptions.palette.setCurrentColorGroup(
            QGuiApplication::focusWindow() == window() ? QPalette::Active : QPalette::Inactive);

    if (activeTitleBarControl() != SC_None)
        titleBarOptions.subControls |= activeTitleBarControl();

    if (!window()->title().isEmpty())
        titleBarOptions.titleBarOptionsString = window()->title();

    titleBarOptions.windowIcon = window()->icon();

    return titleBarOptions;
}

QRect QWasmWindow::normalGeometry() const
{
    return m_normalGeometry;
}

qreal QWasmWindow::devicePixelRatio() const
{
    return screen()->devicePixelRatio();
}

void QWasmWindow::requestUpdate()
{
    m_compositor->requestUpdateWindow(this, QWasmCompositor::UpdateRequestDelivery);
}

bool QWasmWindow::hasTitleBar() const
{
    Qt::WindowFlags flags = window()->flags();
    return !(m_windowState & Qt::WindowFullScreen)
        && flags.testFlag(Qt::WindowTitleHint)
        && !(windowIsPopupType(flags))
        && m_needsCompositor;
}

bool QWasmWindow::windowIsPopupType(Qt::WindowFlags flags) const
{
    if (flags.testFlag(Qt::Tool))
        return false; // Qt::Tool has the Popup bit set but isn't

    return (flags.testFlag(Qt::Popup));
}

void QWasmWindow::requestActivateWindow()
{
    QWindow *modalWindow;
    if (QGuiApplicationPrivate::instance()->isWindowBlocked(window(), &modalWindow)) {
        static_cast<QWasmWindow *>(modalWindow->handle())->requestActivateWindow();
        return;
    }

    if (window()->isTopLevel())
        raise();
    QPlatformWindow::requestActivateWindow();
}

bool QWasmWindow::setMouseGrabEnabled(bool grab)
{
    if (grab)
        m_compositor->setCapture(this);
    else
        m_compositor->releaseCapture();
    return true;
}

QT_END_NAMESPACE
