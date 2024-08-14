// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui>

#if QT_CONFIG(metal) || defined(Q_OS_WIN) || QT_CONFIG(xcb) || defined(ANDROID)
#include "../../shared/nativewindow.h"
#define HAVE_NATIVE_WINDOW
#endif

#include <QDebug>

class TestWindow : public QRasterWindow
{
public:
    using QRasterWindow::QRasterWindow;
    TestWindow(const QBrush &brush) : m_brush(brush) {}

protected:
    void mousePressEvent(QMouseEvent *) override
    {
        m_pressed = true;
        update();
    }

    void mouseReleaseEvent(QMouseEvent *) override
    {
        m_pressed = false;
        update();
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        if (!mask().isNull())
            painter.setClipRegion(mask());
        painter.fillRect(QRect(0, 0, width(), height()),
            m_pressed ? QGradient(QGradient::JuicyPeach) : m_brush);
    }

private:
    QBrush m_brush = QGradient(QGradient::DustyGrass);
    bool m_pressed = false;
};

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    TestWindow window{QGradient(QGradient::WinterNeva)};
    window.resize(500, 500);

    TestWindow *opaqueChildWindow = new TestWindow;
    opaqueChildWindow->setParent(&window);
    opaqueChildWindow->setGeometry(50, 50, 100, 100);
    opaqueChildWindow->showNormal();

    TestWindow *maskedChildWindow = new TestWindow;
    maskedChildWindow->setParent(&window);
    maskedChildWindow->setGeometry(200, 50, 100, 100);
    maskedChildWindow->setMask(QRegion(0, 0, 100, 100, QRegion::Ellipse));
    maskedChildWindow->showNormal();

    static const QColor transparentGreen = QColor(0, 255, 0, 20);
    TestWindow *transparentChildWindow = new TestWindow(transparentGreen);
    // The default surface format of a platform may not include
    // an alpha, so set it explicitly.
    QSurfaceFormat format = transparentChildWindow->format();
    format.setAlphaBufferSize(8);
    transparentChildWindow->setFormat(format);
    // FIXME: Windows requires this, even for child windows
    transparentChildWindow->setFlag(Qt::FramelessWindowHint);
    transparentChildWindow->setParent(&window);
    transparentChildWindow->setGeometry(350, 50, 100, 100);
    transparentChildWindow->showNormal();

#if defined(HAVE_NATIVE_WINDOW)
    NativeWindow nativeWindow;
    if (QWindow *foreignWindow = QWindow::fromWinId(nativeWindow)) {
        foreignWindow->setParent(&window);
        foreignWindow->setGeometry(50, 200, 100, 100);
        foreignWindow->showNormal();
    }

    NativeWindow maskedNativeWindow;
    if (QWindow *foreignWindow = QWindow::fromWinId(maskedNativeWindow)) {
        foreignWindow->setParent(&window);
        foreignWindow->setGeometry(200, 200, 100, 100);
        foreignWindow->setMask(QRegion(0, 0, 100, 100, QRegion::Ellipse));
        foreignWindow->showNormal();
    }

    NativeWindow nativeParentWindow;
    if (QWindow *foreignWindow = QWindow::fromWinId(nativeParentWindow)) {

#ifdef Q_OS_WIN
        // Native parent windows should have WS_CLIPCHILDREN style set
        // to prevent overdrawing child area and cause flickering.
        const HWND hwnd = reinterpret_cast<HWND>(foreignWindow->winId());
        const LONG_PTR oldStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
        SetWindowLongPtr(hwnd, GWL_STYLE, oldStyle | WS_CLIPCHILDREN);
#endif

        foreignWindow->setParent(&window);
        foreignWindow->setGeometry(50, 350, 100, 100);
        foreignWindow->showNormal();

        TestWindow *maskedChildWindowOfNativeWindow = new TestWindow;
        maskedChildWindowOfNativeWindow->setParent(foreignWindow);
        maskedChildWindowOfNativeWindow->setGeometry(25, 25, 50, 50);
        maskedChildWindowOfNativeWindow->showNormal();
    }
#endif

    window.show();

    return app.exec();
}
