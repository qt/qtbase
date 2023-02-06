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

#include <tst_foreignwindow.moc>
QTEST_MAIN(tst_ForeignWindow)
