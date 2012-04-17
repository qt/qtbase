/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINDOWSWINDOW_H
#define QWINDOWSWINDOW_H

#include "qtwindows_additional.h"
#include "qwindowscursor.h"

#include <qpa/qplatformwindow.h>

QT_BEGIN_NAMESPACE

class QWindowsOleDropTarget;
class QDebug;

struct QWindowsGeometryHint
{
    QWindowsGeometryHint() {}
    explicit QWindowsGeometryHint(const QWindow *w);
    static QMargins frame(DWORD style, DWORD exStyle);
    void applyToMinMaxInfo(DWORD style, DWORD exStyle, MINMAXINFO *mmi) const;
    void applyToMinMaxInfo(HWND hwnd, MINMAXINFO *mmi) const;
    bool validSize(const QSize &s) const;

    static inline QPoint mapToGlobal(HWND hwnd, const QPoint &);
    static inline QPoint mapToGlobal(const QWindow *w, const QPoint &);
    static inline QPoint mapFromGlobal(const HWND hwnd, const QPoint &);
    static inline QPoint mapFromGlobal(const QWindow *w, const QPoint &);

    static bool positionIncludesFrame(const QWindow *w);

    QSize minimumSize;
    QSize maximumSize;
};

struct QWindowCreationContext
{
    QWindowCreationContext(const QWindow *w, const QRect &r,
                           DWORD style, DWORD exStyle);

    void applyToMinMaxInfo(MINMAXINFO *mmi) const
        { geometryHint.applyToMinMaxInfo(style, exStyle, mmi); }

    QWindowsGeometryHint geometryHint;
    DWORD style;
    DWORD exStyle;
    QRect requestedGeometry;
    QRect obtainedGeometry;
    QMargins margins;
    int frameX; // Passed on to CreateWindowEx(), including frame.
    int frameY;
    int frameWidth;
    int frameHeight;
};

class QWindowsWindow : public QPlatformWindow
{
public:
    enum Flags
    {
        WithinWmPaint = 0x1,
        WithinSetParent = 0x2,
        FrameDirty = 0x4,            //! Frame outdated by setStyle, recalculate in next query.
        OpenGLSurface = 0x10,
        OpenGLDoubleBuffered = 0x20,
        OpenGlPixelFormatInitialized = 0x40,
        BlockedByModal = 0x80
    };

    struct WindowData
    {
        WindowData() : hwnd(0) {}

        Qt::WindowFlags flags;
        QRect geometry;
        QMargins frame; // Do not use directly for windows, see FrameDirty.
        HWND hwnd;

        static WindowData create(const QWindow *w,
                                 const WindowData &parameters,
                                 const QString &title);
    };

    QWindowsWindow(QWindow *window, const WindowData &data);
    ~QWindowsWindow();

    virtual void setGeometry(const QRect &rect);
    virtual QRect geometry() const { return m_data.geometry; }

    virtual void setVisible(bool visible);
    bool isVisible() const;
    virtual Qt::WindowFlags setWindowFlags(Qt::WindowFlags flags);
    virtual Qt::WindowState setWindowState(Qt::WindowState state);

    HWND handle() const { return m_data.hwnd; }

    virtual WId winId() const { return WId(m_data.hwnd); }
    virtual void setParent(const QPlatformWindow *window);

    virtual void setWindowTitle(const QString &title);
    virtual void raise();
    virtual void lower();

    void windowEvent(QEvent *event);

    virtual void propagateSizeHints();
    virtual QMargins frameMargins() const;

    virtual void setOpacity(qreal level);
    virtual void requestActivateWindow();

    virtual bool setKeyboardGrabEnabled(bool grab);
    virtual bool setMouseGrabEnabled(bool grab);

    Qt::WindowState windowState_sys() const;
    Qt::WindowStates windowStates_sys() const;

    inline unsigned style() const
        { return GetWindowLongPtr(m_data.hwnd, GWL_STYLE); }
    void setStyle(unsigned s) const;
    inline unsigned exStyle() const
        { return GetWindowLongPtr(m_data.hwnd, GWL_EXSTYLE); }
    void setExStyle(unsigned s) const;

    bool handleWmPaint(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    void handleMoved();
    void handleResized(int wParam);
    void handleShown();
    void handleHidden();

    static inline HWND handleOf(const QWindow *w);
    static inline QWindowsWindow *baseWindowOf(const QWindow *w);
    static QWindow *topLevelOf(QWindow *w);
    static inline void *userDataOf(HWND hwnd);
    static inline void setUserDataOf(HWND hwnd, void *ud);

    HDC getDC();
    void releaseDC();

    void getSizeHints(MINMAXINFO *mmi) const;

    QWindowsWindowCursor cursor() const { return m_cursor; }
    void setCursor(const QWindowsWindowCursor &c);
    void applyCursor();

    QWindowsWindow *childAt(const QPoint &clientPoint,
                                unsigned cwexflags = CWP_SKIPINVISIBLE) const;
    QWindowsWindow *childAtScreenPoint(const QPoint &screenPoint,
                                           unsigned cwexflags = CWP_SKIPINVISIBLE) const;

    static QByteArray debugWindowFlags(Qt::WindowFlags wf);

    inline bool testFlag(unsigned f) const  { return (m_flags & f) != 0; }
    inline void setFlag(unsigned f) const   { m_flags |= f; }
    inline void clearFlag(unsigned f) const { m_flags &= ~f; }

    void setEnabled(bool enabled);
    bool isEnabled() const;

    void alertWindow(int durationMs = 0);
    void stopAlertWindow();

private:
    inline void show_sys() const;
    inline void hide_sys() const;
    inline void setGeometry_sys(const QRect &rect) const;
    inline QRect frameGeometry_sys() const;
    inline QRect geometry_sys() const;
    inline WindowData setWindowFlags_sys(Qt::WindowFlags wt, unsigned flags = 0) const;
    inline void setWindowState_sys(Qt::WindowState newState);
    inline void setParent_sys(const QPlatformWindow *parent) const;
    inline void setOpacity_sys(qreal level) const;
    inline void setMouseGrabEnabled_sys(bool grab);
    void destroyWindow();
    void registerDropSite();
    void unregisterDropSite();
    void handleGeometryChange();
    void handleWindowStateChange(Qt::WindowState state);

    mutable WindowData m_data;
    mutable unsigned m_flags;
    HDC m_hdc;
    Qt::WindowState m_windowState;
    qreal m_opacity;
    bool m_mouseGrab;
    QWindowsWindowCursor m_cursor;
    QWindowsOleDropTarget *m_dropTarget;
    unsigned m_savedStyle;
    QRect m_savedFrameGeometry;
};

// Conveniences for window frames.
inline QRect operator+(const QRect &r, const QMargins &m)
{
    return r.adjusted(-m.left(), -m.top(), m.right(), m.bottom());
}

inline QRect operator-(const QRect &r, const QMargins &m)
{
    return r.adjusted(m.left(), m.top(), -m.right(), -m.bottom());
}

// Debug
QDebug operator<<(QDebug d, const RECT &r);
QDebug operator<<(QDebug d, const MINMAXINFO &i);
QDebug operator<<(QDebug d, const NCCALCSIZE_PARAMS &p);

// ---------- QWindowsGeometryHint inline functions.
QPoint QWindowsGeometryHint::mapToGlobal(HWND hwnd, const QPoint &qp)
{
    POINT p = { qp.x(), qp.y() };
    ClientToScreen(hwnd, &p);
    return QPoint(p.x, p.y);
}

QPoint QWindowsGeometryHint::mapFromGlobal(const HWND hwnd, const QPoint &qp)
{
    POINT p = { qp.x(), qp.y() };
    ScreenToClient(hwnd, &p);
    return QPoint(p.x, p.y);
}

QPoint QWindowsGeometryHint::mapToGlobal(const QWindow *w, const QPoint &p)
    { return QWindowsGeometryHint::mapToGlobal(QWindowsWindow::handleOf(w), p); }

QPoint QWindowsGeometryHint::mapFromGlobal(const QWindow *w, const QPoint &p)
    { return QWindowsGeometryHint::mapFromGlobal(QWindowsWindow::handleOf(w), p); }


// ---------- QWindowsBaseWindow inline functions.

QWindowsWindow *QWindowsWindow::baseWindowOf(const QWindow *w)
{
    if (w)
        if (QPlatformWindow *pw = w->handle())
            return static_cast<QWindowsWindow *>(pw);
    return 0;
}

HWND QWindowsWindow::handleOf(const QWindow *w)
{
    if (const QWindowsWindow *bw = QWindowsWindow::baseWindowOf(w))
        return bw->handle();
    return 0;
}

void *QWindowsWindow::userDataOf(HWND hwnd)
{
    return (void *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

void QWindowsWindow::setUserDataOf(HWND hwnd, void *ud)
{
    SetWindowLongPtr(hwnd, GWLP_USERDATA, LONG_PTR(ud));
}

QT_END_NAMESPACE

#endif // QWINDOWSWINDOW_H
