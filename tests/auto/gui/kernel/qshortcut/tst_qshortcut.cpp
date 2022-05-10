// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QtGui/qguiapplication.h>
#include <QtGui/qshortcut.h>
#include <QtGui/qpainter.h>
#include <QtGui/qrasterwindow.h>
#include <QtGui/qscreen.h>
#include <QtGui/qwindow.h>

class tst_QShortcut : public QObject
{
    Q_OBJECT
public:

private slots:
    void trigger();
};

class ColoredWindow : public QRasterWindow {
public:
    ColoredWindow(QColor c) : m_color(c) {}

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    const QColor m_color;
};

void ColoredWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(QRect(QPoint(), size()), m_color);
}

static void sendKey(QWindow *target, Qt::Key k, char c, Qt::KeyboardModifiers modifiers)
{
    QTest::sendKeyEvent(QTest::Press, target, k, c, modifiers);
    QTest::sendKeyEvent(QTest::Release, target, k, c, modifiers);
}

void tst_QShortcut::trigger()
{
    ColoredWindow w(Qt::yellow);
    w.setTitle(QTest::currentTestFunction());
    w.resize(QGuiApplication::primaryScreen()->size() / 4);
    new QShortcut(Qt::CTRL | Qt::Key_Q, &w, SLOT(close()));
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QTRY_VERIFY(QGuiApplication::applicationState() == Qt::ApplicationActive);
    sendKey(&w, Qt::Key_Q, 'q', Qt::ControlModifier);
    QTRY_VERIFY(!w.isVisible());
}

QTEST_MAIN(tst_QShortcut)
#include "tst_qshortcut.moc"
