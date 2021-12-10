/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qwindow.h>
#include <qbackingstore.h>
#include <qpainter.h>

#include <QtTest/QtTest>

#include <QEvent>

// For QSignalSpy slot connections.
Q_DECLARE_METATYPE(Qt::ScreenOrientation)

class tst_QBackingStore : public QObject
{
    Q_OBJECT

private slots:
    void flush();

    void scrollRectInImage_data();
    void scrollRectInImage();
};

void tst_QBackingStore::scrollRectInImage_data()
{
    QTest::addColumn<QRect>("rect");
    QTest::addColumn<QPoint>("offset");

    QTest::newRow("empty rect") << QRect() << QPoint();
    QTest::newRow("rect outside image") << QRect(-100, -100, 1000, 1000) << QPoint(10, 10);
    QTest::newRow("scroll outside positive") << QRect(10, 10, 10, 10) << QPoint(1000, 1000);
    QTest::newRow("scroll outside negative") << QRect(10, 10, 10, 10) << QPoint(-1000, -1000);

    QTest::newRow("sub-rect positive scroll") << QRect(100, 100, 50, 50) << QPoint(10, 10);
    QTest::newRow("sub-rect negative scroll") << QRect(100, 100, 50, 50) << QPoint(-10, -10);

    QTest::newRow("positive vertical only") << QRect(100, 100, 50, 50) << QPoint(0, 10);
    QTest::newRow("negative vertical only") << QRect(100, 100, 50, 50) << QPoint(0, -10);
    QTest::newRow("positive horizontal only") << QRect(100, 100, 50, 50) << QPoint(10, 0);
    QTest::newRow("negative horizontal only") << QRect(100, 100, 50, 50) << QPoint(-10, 0);

    QTest::newRow("whole rect positive") << QRect(0, 0, 250, 250) << QPoint(10, 10);
    QTest::newRow("whole rect negative") << QRect(0, 0, 250, 250) << QPoint(-10, -10);
}

QT_BEGIN_NAMESPACE
Q_GUI_EXPORT void qt_scrollRectInImage(QImage &, const QRect &, const QPoint &);
QT_END_NAMESPACE

void tst_QBackingStore::scrollRectInImage()
{
    QImage test(250, 250, QImage::Format_ARGB32_Premultiplied);

    QFETCH(QRect, rect);
    QFETCH(QPoint, offset);

    qt_scrollRectInImage(test, rect, offset);
}

class Window : public QWindow
{
public:
    Window()
        : backingStore(this)
    {
    }

    void resizeEvent(QResizeEvent *)
    {
        backingStore.resize(size());
    }

    void exposeEvent(QExposeEvent *event)
    {
        QRect rect(QPoint(), size());

        backingStore.beginPaint(rect);

        QPainter p(backingStore.paintDevice());
        p.fillRect(rect, Qt::white);
        p.end();

        backingStore.endPaint();

        backingStore.flush(event->region().boundingRect());
    }

private:
    QBackingStore backingStore;
};

void tst_QBackingStore::flush()
{
    Window window;
    window.setGeometry(20, 20, 200, 200);
    window.showMaximized();

    QTRY_VERIFY(window.isExposed());
}

#include <tst_qbackingstore.moc>
QTEST_MAIN(tst_QBackingStore);
