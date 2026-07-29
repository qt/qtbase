// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/qwindow.h>

#include <QtCore/qt_windows.h>

using namespace Qt::StringLiterals;

// The Windows QPA appends a UUID to a window class name that another Qt
// instance in the same process has already registered, so that each instance
// dispatches window messages to its own window procedure. These tests
// impersonate that other instance by registering the class names the platform
// plugin is about to use, which is all QWindowsWindowClassRegistry looks at.

class tst_QWindowsWindowClassRegistry : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void decoratesClashingWindowClassName();
    void keepsWindowClassNameWhenProcedureMatches();

private:
    QString m_classNamePrefix;
    WNDPROC m_windowProcedure = nullptr;
};

static QString nativeClassName(WId winId)
{
    wchar_t buffer[256];
    const int length = GetClassName(reinterpret_cast<HWND>(winId), buffer, 256);
    return QString::fromWCharArray(buffer, length);
}

static LRESULT CALLBACK foreignWindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hwnd, message, wParam, lParam);
}

static bool registerForeignWindowClass(const QString &name, WNDPROC procedure)
{
    WNDCLASSEX wc{};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = procedure;
    wc.hInstance = static_cast<HINSTANCE>(GetModuleHandle(nullptr));
    wc.lpszClassName = reinterpret_cast<LPCWSTR>(name.utf16());

    return RegisterClassEx(&wc) != 0;
}

void tst_QWindowsWindowClassRegistry::initTestCase()
{
    if (QGuiApplication::platformName() != "windows"_L1)
        QSKIP("This test requires the windows platform plugin");

    // Derive the version and namespace prefix from a window class this Qt
    // instance has registered itself, rather than recomputing it here.
    QWindow probe;
    probe.create();
    const QString probeClassName = nativeClassName(probe.winId());

    const qsizetype tokenIndex = probeClassName.indexOf("QWindow"_L1);
    QVERIFY2(tokenIndex > 0, qPrintable("Unexpected window class name "_L1 + probeClassName));
    m_classNamePrefix = probeClassName.first(tokenIndex);

    // Pin the class name scheme the tests below rely on. Should it change, this
    // fails instead of the tests silently registering names Qt no longer uses.
    QCOMPARE(probeClassName, m_classNamePrefix + "QWindowIcon"_L1);

    m_windowProcedure =
        reinterpret_cast<WNDPROC>(GetClassLongPtr(reinterpret_cast<HWND>(probe.winId()),
                                                  GCLP_WNDPROC));
    QVERIFY(m_windowProcedure);
}

void tst_QWindowsWindowClassRegistry::decoratesClashingWindowClassName()
{
    // Occupy the class name a Qt::Tool window is about to be registered under,
    // using a window procedure that is not this instance's.
    const QString clashingName = m_classNamePrefix + "QWindowToolSaveBits"_L1;
    QVERIFY2(registerForeignWindowClass(clashingName, foreignWindowProcedure),
             "Failed to register the window class to clash with");

    QWindow tool;
    tool.setFlags(Qt::Tool);
    tool.resize(200, 100);
    tool.show();

    const QString toolClassName = nativeClassName(tool.winId());
    QVERIFY2(toolClassName != clashingName,
             "The clashing window class name was not de-duplicated");
    QVERIFY2(toolClassName.startsWith(clashingName),
             qPrintable("Unexpected window class name "_L1 + toolClassName));

    // Had the window been created against the foreign class, its messages would
    // be dispatched to foreignWindowProcedure() and it would never be exposed.
    QVERIFY(QTest::qWaitForWindowExposed(&tool));
}

void tst_QWindowsWindowClassRegistry::keepsWindowClassNameWhenProcedureMatches()
{
    // A class registered with this instance's own window procedure is not a
    // clash - messages already arrive where they should - so the class name has
    // to be used unmodified.
    const QString name = m_classNamePrefix + "QWindowToolTipSaveBits"_L1;
    QVERIFY2(registerForeignWindowClass(name, m_windowProcedure),
             "Failed to register the window class to clash with");

    QWindow toolTip;
    toolTip.setFlags(Qt::ToolTip);
    toolTip.resize(200, 100);
    toolTip.show();

    QCOMPARE(nativeClassName(toolTip.winId()), name);
}

QTEST_MAIN(tst_QWindowsWindowClassRegistry)

#include "tst_qwindowswindowclassregistry.moc"
