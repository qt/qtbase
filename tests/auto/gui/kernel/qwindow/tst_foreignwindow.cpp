// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>

#include <QtCore/qloggingcategory.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

#if defined(Q_OS_MACOS)
#  include <AppKit/AppKit.h>
#elif defined(Q_OS_WIN)
#  include <winuser.h>
#endif

Q_LOGGING_CATEGORY(lcTests, "qt.gui.tests")

class NativeWindow
{
    Q_DISABLE_COPY(NativeWindow)
public:
    NativeWindow();
    ~NativeWindow();

    operator WId() const { return reinterpret_cast<WId>(m_handle); }

    void setGeometry(const QRect &rect);
    QRect geometry() const;

private:
#if defined(Q_OS_MACOS)
    NSView *m_handle = nullptr;
#elif defined(Q_OS_WIN)
    HWND m_handle = nullptr;
#endif
};

#if defined(Q_OS_MACOS)

@interface View : NSView
@end

@implementation View
- (instancetype)init
{
    if ((self = [super init])) {
        qCDebug(lcTests) << "Initialized" << self;
    }
    return self;
}

- (void)dealloc
{
    qCDebug(lcTests) << "Deallocating" << self;
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

#elif defined(Q_OS_WIN)

NativeWindow::NativeWindow()
{
    static const LPCWSTR className = []{
        WNDCLASS wc = {};
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"Native Window";
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

#endif

class tst_ForeignWindow: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        auto *platformIntegration = QGuiApplicationPrivate::platformIntegration();
        if (!platformIntegration->hasCapability(QPlatformIntegration::ForeignWindows))
            QSKIP("This platform does not support foreign windows");
    }

    void fromWinId();
    void initialState();
};

void tst_ForeignWindow::fromWinId()
{
    NativeWindow nativeWindow;
    QVERIFY(nativeWindow);

    std::unique_ptr<QWindow> foreignWindow(QWindow::fromWinId(nativeWindow));
    QVERIFY(foreignWindow);
    QVERIFY(foreignWindow->flags().testFlag(Qt::ForeignWindow));
    QVERIFY(foreignWindow->handle());

    // fromWinId does not take (exclusive) ownership of the native window,
    // so deleting the foreign window should not be a problem/cause crashes.
    foreignWindow.reset();
}

void tst_ForeignWindow::initialState()
{
    NativeWindow nativeWindow;
    QVERIFY(nativeWindow);

    // A foreign window can be used to embed a Qt UI in a foreign window hierarchy,
    // in which case the foreign window merely acts as a parent and should not be
    // modified, or to embed a foreign window in a Qt UI, in which case the foreign
    // window must to be able to re-parent, move, resize, show, etc, so that the
    // containing Qt UI can treat it as any other window.

    // At the point of creation though, we don't know what the foreign window
    // will be used for, so the platform should not assume it can modify the
    // window. Any properties set on the native window should persist past
    // creation of the foreign window.

    const QRect initialGeometry(123, 456, 321, 654);
    nativeWindow.setGeometry(initialGeometry);

    std::unique_ptr<QWindow> foreignWindow(QWindow::fromWinId(nativeWindow));
    QCOMPARE(nativeWindow.geometry(), initialGeometry);

    // For extra bonus points, the foreign window should actually
    // reflect the state of the native window.
    QCOMPARE(foreignWindow->geometry(), initialGeometry);
}

#include <tst_foreignwindow.moc>
QTEST_MAIN(tst_ForeignWindow)
