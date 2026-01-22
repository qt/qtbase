// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "nativewindow.h"

#include <QtCore/qscopeguard.h>

#if defined(Q_OS_WIN)

struct ScopedDpiAwareness {
    ScopedDpiAwareness(DPI_AWARENESS_CONTEXT awareness)
    { m_oldAwareness = SetThreadDpiAwarenessContext(awareness); }
    ~ScopedDpiAwareness() { SetThreadDpiAwarenessContext(m_oldAwareness); }
private:
    DPI_AWARENESS_CONTEXT m_oldAwareness;
};

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
    m_handle = CreateWindowEx(0, className, nullptr, WS_POPUP | WS_CLIPCHILDREN,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
}

NativeWindow::~NativeWindow()
{
    DestroyWindow(m_handle);
}

void NativeWindow::setVisible(bool visible)
{
    if (visible && GetAncestor(m_handle, GA_PARENT) == GetDesktopWindow()) {
        RECT windowRect;
        GetWindowRect(m_handle, &windowRect);

        LONG style = GetWindowLong(m_handle, GWL_STYLE);
        style &= ~WS_POPUP;
        style |= WS_OVERLAPPEDWINDOW;
        SetWindowLong(m_handle, GWL_STYLE, style);

        RECT adjustedRect = windowRect;
        AdjustWindowRectEx(&adjustedRect, style, false,
            GetWindowLong(m_handle, GWL_EXSTYLE));

        SetWindowPos(m_handle, nullptr,
            // Adjust x position to keep client area left edge in the same place,
            // to match Qt's apparent behavior. FIXME: Check if Qt is correct.
            adjustedRect.left, windowRect.top,
            adjustedRect.right - adjustedRect.left,
            adjustedRect.bottom - adjustedRect.top,
            SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE
        );
    }
    ShowWindow(m_handle, visible ? SW_SHOW : SW_HIDE);
}

void NativeWindow::setGeometry(const QRect &rect)
{
    ScopedDpiAwareness dpiAwareness(DPI_AWARENESS_CONTEXT_UNAWARE);
    MoveWindow(m_handle, rect.x(), rect.y(), rect.width(), rect.height(), false);
}

QRect NativeWindow::geometry() const
{
    ScopedDpiAwareness dpiAwareness(DPI_AWARENESS_CONTEXT_UNAWARE);

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

bool NativeWindow::isParentOf(WId childWinId)
{
    return GetAncestor(Handle(childWinId), GA_PARENT) == m_handle;
}

void NativeWindow::setParent(WId parent)
{
    SetParent(m_handle, Handle(parent));
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

void NativeWindow::setVisible(bool visible)
{
    if (visible)
        xcb_map_window(connection, m_handle);
    else
        xcb_unmap_window(connection, m_handle);
    xcb_flush(connection);
}

void NativeWindow::setGeometry(const QRect &rect)
{
    const quint32 mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y
            | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
    const qint32 values[] = { rect.x(), rect.y(), rect.width(), rect.height() };
    xcb_configure_window(connection, m_handle, mask,
        reinterpret_cast<const quint32*>(values));

    // Ask the WM to respect out geometry on show
    xcb_size_hints_t hints = {};
    xcb_icccm_size_hints_set_position(&hints, true, rect.x(), rect.y());
    xcb_icccm_size_hints_set_size(&hints, true, rect.width(), rect.height());
    xcb_icccm_set_wm_normal_hints(connection, m_handle, &hints);

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
    return tree ? tree->parent : 0;
}

bool NativeWindow::isParentOf(WId childWinId)
{
    xcb_query_tree_reply_t *tree = xcb_query_tree_reply(
        connection, xcb_query_tree(connection, Handle(childWinId)), nullptr);
    const auto cleanup = qScopeGuard([&]{ free(tree); });
    return tree->parent == m_handle;
}

void NativeWindow::setParent(WId parent)
{
    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;

    xcb_reparent_window(connection, m_handle,
        parent ? Handle(parent) : screen->root, 0, 0);
}

#elif defined (ANDROID)
NativeWindow::NativeWindow()
{
    m_handle = QJniObject::construct<QtJniTypes::View, QtJniTypes::Context>(
                                                QNativeInterface::QAndroidApplication::context());
    m_handle.callMethod<void>("setBackgroundColor", 0xffffaaff);
}

NativeWindow::~NativeWindow()
{
}

void NativeWindow::setVisible(bool visible)
{
    m_handle.callMethod<void>("setVisibility",
        visible ? 0 /* View.VISIBLE */ : 2 /* View.GONE */);
}

NativeWindow::operator WId() const
{
    return reinterpret_cast<WId>(m_handle.object());
}

void NativeWindow::setGeometry(const QRect &rect)
{
    // No-op, the view geometry is handled by the QWindow constructed from it
}

QRect NativeWindow::geometry() const
{
    int x = m_handle.callMethod<jint>("getX");
    int y = m_handle.callMethod<jint>("getY");
    int w = m_handle.callMethod<jint>("getWidth");
    int h = m_handle.callMethod<jint>("getHeight");
    return QRect(x, y, w, h);
}

WId NativeWindow::parentWinId() const
{
    // TODO note, the returned object is a ViewParent, not necessarily
    // a View - what is this used for?
    using namespace QtJniTypes;
    ViewParent parentView = m_handle.callMethod<ViewParent>("getParent");
    if (parentView.isValid())
        return reinterpret_cast<WId>(parentView.object());
    return 0L;
}

#endif
