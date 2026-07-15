// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtCore/qoperatingsystemversion.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qstylefactory.h>
#include <QtWidgets/qwidget.h>

#include <qt_windows.h>
#include <dwmapi.h>

using namespace Qt::StringLiterals;

#ifdef Q_CC_MSVC
static constexpr auto dwmaWindowCornerPreference = DWMWA_WINDOW_CORNER_PREFERENCE;
static constexpr uint32_t dwmcpDefault = DWMWCP_DEFAULT;
static constexpr uint32_t dwmcpRound = DWMWCP_ROUND;
#else
// MinGW 13.1.0 does not provide this
static constexpr auto dwmaWindowCornerPreference = 33;
static constexpr uint32_t dwmcpDefault = 0;
static constexpr uint32_t dwmcpRound = 2;
#endif

static uint32_t cornerPreference(const QWidget *widget)
{
    uint32_t pref = 0;
    const auto hwnd = reinterpret_cast<HWND>(const_cast<QWidget *>(widget)->winId());
    if (FAILED(DwmGetWindowAttribute(hwnd, dwmaWindowCornerPreference, &pref, sizeof(pref))))
        return uint32_t(-1);
    return pref;
}

class tst_QWindows11Style : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cornerPreferenceOnFullScreen();
    void cornerPreferenceOnShowFullScreen();
};

void tst_QWindows11Style::initTestCase()
{
    if (QGuiApplication::platformName() != QLatin1StringView("windows"))
        QSKIP("This test requires the Windows platform plugin.");
    if (QOperatingSystemVersion::current() < QOperatingSystemVersion::Windows11_21H2)
        QSKIP("Native rounded top level windows require Windows 11.");
    QStyle *style = QStyleFactory::create(u"windows11"_s);
    if (!style)
        QSKIP("The windows11 style is not available.");
    QApplication::setStyle(style);
}

// QTBUG-147453: A fullscreen window must not have rounded corners, so the
// corner preference must be DWMWCP_DEFAULT. When the window leaves
// fullscreen, the corners must be rounded again.
void tst_QWindows11Style::cornerPreferenceOnFullScreen()
{
    QWidget widget;
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    if (cornerPreference(&widget) == uint32_t(-1))
        QSKIP("DwmGetWindowAttribute does not support the corner preference.");
    QTRY_COMPARE(cornerPreference(&widget), dwmcpRound);

    widget.showFullScreen();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QTRY_COMPARE(cornerPreference(&widget), dwmcpDefault);

    widget.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QTRY_COMPARE(cornerPreference(&widget), dwmcpRound);
}

void tst_QWindows11Style::cornerPreferenceOnShowFullScreen()
{
    QWidget widget;
    widget.showFullScreen();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    if (cornerPreference(&widget) == uint32_t(-1))
        QSKIP("DwmGetWindowAttribute does not support the corner preference.");
    QTRY_COMPARE(cornerPreference(&widget), dwmcpDefault);

    widget.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QTRY_COMPARE(cornerPreference(&widget), dwmcpRound);
}

QTEST_MAIN(tst_QWindows11Style)
#include "tst_qwindows11style.moc"
