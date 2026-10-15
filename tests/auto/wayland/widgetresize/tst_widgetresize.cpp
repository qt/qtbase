// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mockcompositor.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>

using namespace MockCompositor;

class tst_widgetresize : public QObject, private DefaultCompositor
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanup() { QTRY_VERIFY2(isClean(), qPrintable(dirtyMessage())); }
    void floatingDockWidget_data();
    void floatingDockWidget();
};

void tst_widgetresize::initTestCase()
{
    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");
}

void tst_widgetresize::floatingDockWidget_data()
{
    QTest::addColumn<Qt::Edges>("edges");
    QTest::addColumn<uint>("expectedEdges");

    QTest::newRow("Center") << Qt::Edges() << uint(XdgToplevel::resize_edge_none);
    QTest::newRow("Top") << Qt::Edges(Qt::TopEdge) << uint(XdgToplevel::resize_edge_top);
    QTest::newRow("Bottom") << Qt::Edges(Qt::BottomEdge) << uint(XdgToplevel::resize_edge_bottom);
    QTest::newRow("Left") << Qt::Edges(Qt::LeftEdge) << uint(XdgToplevel::resize_edge_left);
    QTest::newRow("Right") << Qt::Edges(Qt::RightEdge) << uint(XdgToplevel::resize_edge_right);
    QTest::newRow("TopLeft") << (Qt::TopEdge | Qt::LeftEdge)
                             << uint(XdgToplevel::resize_edge_top_left);
    QTest::newRow("TopRight") << (Qt::TopEdge | Qt::RightEdge)
                              << uint(XdgToplevel::resize_edge_top_right);
    QTest::newRow("BottomLeft") << (Qt::BottomEdge | Qt::LeftEdge)
                                << uint(XdgToplevel::resize_edge_bottom_left);
    QTest::newRow("BottomRight") << (Qt::BottomEdge | Qt::RightEdge)
                                 << uint(XdgToplevel::resize_edge_bottom_right);
}

// A Wayland client can't position its toplevels. Resizing a floating dock widget from
// its top or left edge must therefore be delegated to the compositor (QTBUG-136715).
void tst_widgetresize::floatingDockWidget()
{
    QFETCH(const Qt::Edges, edges);
    QFETCH(const uint, expectedEdges);

    QMainWindow mainWindow;
    auto *dock = new QDockWidget("Dock", &mainWindow);
    dock->setWidget(new QLabel("Dock content"));
    mainWindow.addDockWidget(Qt::LeftDockWidgetArea, dock);
    dock->setFloating(true);
    dock->resize(200, 200);
    dock->show();

    QCOMPOSITOR_TRY_VERIFY(xdgToplevel());
    QSignalSpy configureSpy(exec([&] { return xdgSurface(); }), &XdgSurface::configureCommitted);
    exec([&] { xdgToplevel()->sendCompleteConfigure(); });
    QTRY_COMPARE(configureSpy.size(), 1);
    QTRY_VERIFY(dock->windowHandle()->isExposed());

    // Press on the outermost pixel of the requested edge, i.e. on the dock widget's frame
    const QSize size = dock->size();
    QPointF pos(size.width() / 2, size.height() / 2);
    if (edges & Qt::LeftEdge)
        pos.setX(0);
    else if (edges & Qt::RightEdge)
        pos.setX(size.width() - 1);
    if (edges & Qt::TopEdge)
        pos.setY(0);
    else if (edges & Qt::BottomEdge)
        pos.setY(size.height() - 1);

    exec([&] {
        auto *surface = xdgToplevel()->surface();
        auto *p = pointer();
        auto *c = client();
        p->sendEnter(surface, pos);
        p->sendFrame(c);
        p->sendButton(c, BTN_LEFT, Pointer::button_state_pressed);
        p->sendFrame(c);
        p->sendButton(c, BTN_LEFT, Pointer::button_state_released);
        p->sendFrame(c);
    });

    if (edges) {
        QCOMPOSITOR_TRY_COMPARE(xdgToplevel()->m_resizeRequests, 1);
        QCOMPOSITOR_COMPARE(xdgToplevel()->m_resizeEdges, expectedEdges);
    } else {
        // Make sure the press has been processed before checking that nothing happened
        QTRY_VERIFY(!(QGuiApplication::mouseButtons() & Qt::LeftButton));
        QCOMPOSITOR_COMPARE(xdgToplevel()->m_resizeRequests, 0);
    }
}

int main(int argc, char **argv)
{
    QTemporaryDir tmpRuntimeDir;
    qputenv("XDG_RUNTIME_DIR", tmpRuntimeDir.path().toLocal8Bit());
    qputenv("XDG_CURRENT_DESKTOP", "qtwaylandtests");
    qputenv("QT_QPA_PLATFORM", "wayland");
    tst_widgetresize tc;
    QApplication app(argc, argv);
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_widgetresize.moc"
