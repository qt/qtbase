// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef NATIVEWINDOW_H
#define NATIVEWINDOW_H

#if defined(Q_OS_MACOS)
#  include <AppKit/AppKit.h>
#  define VIEW_BASE NSView
#elif defined(Q_OS_IOS)
#  include <UIKit/UIKit.h>
#  define VIEW_BASE UIView
#elif defined(Q_OS_WIN)
#  include <winuser.h>
#elif QT_CONFIG(xcb)
#  include <xcb/xcb.h>
#endif

class NativeWindow
{
    Q_DISABLE_COPY(NativeWindow)
public:
    NativeWindow();
    ~NativeWindow();

    operator WId() const;
    WId parentWinId() const;

    void setGeometry(const QRect &rect);
    QRect geometry() const;

private:
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    VIEW_BASE *m_handle = nullptr;
#elif defined(Q_OS_WIN)
    HWND m_handle = nullptr;
#elif QT_CONFIG(xcb)
    xcb_window_t m_handle = 0;
#endif
};

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)

@interface View : VIEW_BASE
@end

@implementation View
- (instancetype)init
{
    if ((self = [super init])) {
#if defined(Q_OS_MACOS)
        self.wantsLayer = YES;
#endif
        self.layer.backgroundColor = CGColorCreateGenericRGB(1.0, 0.5, 1.0, 1.0);
    }
    return self;
}

- (void)dealloc
{
    [super dealloc];
}
@end

NativeWindow::NativeWindow()
    : m_handle([View new])
{
}

NativeWindow::~NativeWindow()
{
    [m_handle release];
}

void NativeWindow::setGeometry(const QRect &rect)
{
    m_handle.frame = QRectF(rect).toCGRect();
}

QRect NativeWindow::geometry() const
{
    return QRectF::fromCGRect(m_handle.frame).toRect();
}

NativeWindow::operator WId() const
{
    return reinterpret_cast<WId>(m_handle);
}

WId NativeWindow::parentWinId() const
{
    return WId(m_handle.superview);
}

#elif defined(Q_OS_WIN)

NativeWindow::NativeWindow()
{
    static const LPCWSTR className = []{
        WNDCLASS wc = {};
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"Native Window";
        wc.hbrBackground = CreateSolidBrush(RGB(255, 128, 255));
        RegisterClass(&wc);
        return wc.lpszClassName;
    }();
    m_handle = CreateWindowEx(0, className, nullptr, WS_POPUP,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
}

NativeWindow::~NativeWindow()
{
    DestroyWindow(m_handle);
}

void NativeWindow::setGeometry(const QRect &rect)
{
    MoveWindow(m_handle, rect.x(), rect.y(), rect.width(), rect.height(), false);
}

QRect NativeWindow::geometry() const
{
    WINDOWPLACEMENT wp;
    wp.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(m_handle, &wp)) {
        RECT r = wp.rcNormalPosition;
        return QRect(r.left, r.top, r.right - r.left, r.bottom - r.top);
    }
    return {};
}

NativeWindow::operator WId() const
{
    return reinterpret_cast<WId>(m_handle);
}

WId NativeWindow::parentWinId() const
{
    return WId(GetAncestor(m_handle, GA_PARENT));
}

#elif QT_CONFIG(xcb)

struct Connection
{
    Connection() : m_connection(xcb_connect(nullptr, nullptr)) {}
    ~Connection() { xcb_disconnect(m_connection); }
    operator xcb_connection_t*() const { return m_connection; }
    xcb_connection_t *m_connection = nullptr;
};

static Connection connection;

NativeWindow::NativeWindow()
{
    m_handle = xcb_generate_id(connection);

    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;

    xcb_create_window(connection, XCB_COPY_FROM_PARENT, m_handle,
        screen->root, 0, 0, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen->root_visual, XCB_CW_BACK_PIXEL,
        (const uint32_t []){ 0xffffaaff });

    xcb_flush(connection);
}

NativeWindow::~NativeWindow()
{
    xcb_destroy_window(connection, m_handle);
}

void NativeWindow::setGeometry(const QRect &rect)
{
    const quint32 mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y
            | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
    const qint32 values[] = { rect.x(), rect.y(), rect.width(), rect.height() };
    xcb_configure_window(connection, m_handle, mask,
        reinterpret_cast<const quint32*>(values));
    xcb_flush(connection);
}

QRect NativeWindow::geometry() const
{
    xcb_get_geometry_reply_t *geometry = xcb_get_geometry_reply(
        connection, xcb_get_geometry(connection, m_handle), nullptr);
    const auto cleanup = qScopeGuard([&]{ free(geometry); });
    return QRect(geometry->x, geometry->y, geometry->width, geometry->height);
}

NativeWindow::operator WId() const
{
    return m_handle;
}

WId NativeWindow::parentWinId() const
{
    xcb_query_tree_reply_t *tree = xcb_query_tree_reply(
        connection, xcb_query_tree(connection, m_handle), nullptr);
    const auto cleanup = qScopeGuard([&]{ free(tree); });
    return tree->parent;
}

#endif

#endif // NATIVEWINDOW_H
